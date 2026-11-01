"""Check command isolation and quoting without starting a graphical terminal."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class LauncherTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="phosphor launcher ")
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)
        self.capture = self.folder / "arguments.json"
        self.binary = self.folder / "ghostty stub"
        self.binary.write_text(
            f"#!{sys.executable}\n"
            "import json, os, pathlib, sys\n"
            "pathlib.Path(os.environ['PHOSPHOR_CAPTURE']).write_text("
            "json.dumps({'args': sys.argv[1:], 'history': os.environ.get('HISTFILE')}))\n"
        )
        self.binary.chmod(0o700)
        self.env = dict(os.environ, GHOSTTY=str(self.binary), PHOSPHOR_CAPTURE=str(self.capture))

    def run_mode(self, *args):
        return subprocess.run(["bash", str(ROOT / "run.sh"), *args], env=self.env,
                              cwd=self.folder, capture_output=True, text=True, timeout=5)

    def captured(self):
        return json.loads(self.capture.read_text())

    def test_baseline_uses_only_isolated_config(self):
        self.assertEqual(self.run_mode("baseline").returncode, 0)
        args = self.captured()["args"]
        self.assertIn("--config-default-files=false", args)
        self.assertIn("--gtk-single-instance=false", args)
        self.assertIn("--class=org.phosphor.Spike001", args)
        self.assertEqual([a for a in args if a.startswith("--config-file=")], [f"--config-file={ROOT}/config"])
        self.assertFalse(any(a.startswith("--custom-shader=") for a in args))
        self.assertEqual(args[args.index("-e") + 1:], ["/bin/bash", "--noprofile", "--norc"])
        self.assertEqual(self.captured()["history"], "/dev/null")

    def test_prototype_differs_only_by_shader(self):
        self.run_mode("baseline")
        baseline = self.captured()["args"]
        self.assertEqual(self.run_mode("prototype").returncode, 0)
        prototype = self.captured()["args"]
        shaders = [a for a in prototype if a.startswith("--custom-shader=")]
        self.assertEqual(shaders, [f"--custom-shader={ROOT}/molten-amber.glsl"])
        self.assertEqual([a for a in prototype if a not in shaders], baseline)

    def test_command_arguments_are_not_evaluated(self):
        marker = self.folder / "must not exist"
        command = ["printf", f"$(touch '{marker}')", "two words", ";", "--font-size=99"]
        self.assertEqual(self.run_mode("prototype", "--", *command).returncode, 0)
        args = self.captured()["args"]
        self.assertEqual(args[args.index("-e") + 1:], command)
        self.assertFalse(marker.exists())

    def test_invalid_mode_does_not_start_ghostty(self):
        self.assertEqual(self.run_mode("other").returncode, 2)
        self.assertFalse(self.capture.exists())

    def test_missing_mode_does_not_start_ghostty(self):
        self.assertEqual(self.run_mode().returncode, 2)
        self.assertFalse(self.capture.exists())

    def test_invalid_command_separator_does_not_start_ghostty(self):
        for args in [("baseline", "echo"), ("baseline", "--")]:
            with self.subTest(args=args):
                self.assertEqual(self.run_mode(*args).returncode, 2)
        self.assertFalse(self.capture.exists())

    def test_missing_binary_reports_dependency_without_installing(self):
        self.env["GHOSTTY"] = str(self.folder / "absent")
        result = self.run_mode("prototype")
        self.assertEqual(result.returncode, 127)
        self.assertIn("owner-approved binary", result.stderr)
        self.assertFalse(self.capture.exists())


if __name__ == "__main__":
    unittest.main()
