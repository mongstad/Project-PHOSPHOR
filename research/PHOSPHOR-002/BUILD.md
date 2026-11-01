# Build and validation provenance

2026-11-01, Fedora 44. No privileged/system-wide installation was performed.
Initial PATH lacked CMake and a C++ driver; Qt 6.11.2 runtime was installed.
A later package-file inspection found Clang 20 at
`/usr/lib64/rocm/llvm/bin/clang++` (Fedora rocm-clang package).

Development RPMs were downloaded from official `fedora`/`updates` repositories
with `dnf download`, signature/digest checked with `rpmkeys --checksig`, and
extracted with `rpm2cpio | cpio -idm` under `/tmp/phosphor-002/sysroot`.
The [RPM manifest](evidence/dependency-rpms.sha256) records extracted x86_64 and
noarch packages. Initial download also fetched i686 packages; none were extracted
or linked. Installed Qt runtimes/plugins were linked into the temporary prefix
where CMake required exact versioned target files. The missing CMake librhash
runtime was also extracted locally. No binaries are committed.

The already preserved PHOSPHOR-001 temporary sysroot supplied OpenGL/Vulkan
headers and link names (read-only reuse, no changes). Its provenance remains in
[the previous build record](../PHOSPHOR-001/BUILD.md). The complete successful
CMake configuration is [clang-cache.txt](evidence/clang-cache.txt).

Local reproduction on this machine (requires the extracted files above):

```sh
export LD_LIBRARY_PATH=/tmp/phosphor-002/sysroot/usr/lib64
/tmp/phosphor-002/sysroot/usr/bin/cmake \
  -S prototypes/qt-molten-amber -B /tmp/phosphor-002/clang-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=/usr/lib64/rocm/llvm/bin/clang++ \
  -DCMAKE_PREFIX_PATH='/tmp/phosphor-002/sysroot/usr;/tmp/phosphor-001-native/sysroot/usr' \
  -DCMAKE_CXX_FLAGS='-L/tmp/phosphor-002/sysroot/usr/lib64 -isystem /tmp/phosphor-002/sysroot/usr/include/c++/16 -isystem /tmp/phosphor-002/sysroot/usr/include/c++/16/x86_64-redhat-linux -isystem /tmp/phosphor-002/sysroot/usr/include/c++/16/backward'
/tmp/phosphor-002/sysroot/usr/bin/cmake --build /tmp/phosphor-002/clang-build -j4
QT_FORCE_STDERR_LOGGING=1 /tmp/phosphor-002/sysroot/usr/bin/ctest \
  --test-dir /tmp/phosphor-002/clang-build --output-on-failure
```

The normal development-package build is in the [prototype README](../../prototypes/qt-molten-amber/README.md).
The harmless Clang warning that `-L` is unused during compilation comes from
these local combined compile/link flags; the path is used at link time.

## Compiler discrepancy

GCC 16.2.1 from locally extracted Fedora RPMs built successfully, but optimized
`-O3` and `-O2` renders produced rectangular halo coverage. Instrumented
observations showed an incorrect exponential result; adding diagnostic prints
changed that result. GCC `-O0` passed the expanded tests. Optimized Clang 20
passed them and native captures show the expected finite halo. This is evidence
of an optimization-sensitive discrepancy, **not proof of a GCC compiler bug**:
no minimized upstream reproducer or independent root cause is established.
GCC optimized builds are not validated for this POC. Earlier captures/logs are
preserved in [diagnostics](evidence/diagnostics/) and excluded from final evidence.
The original tests missed transparent borders; the final suite adds that check.

Clang UBSan's normal runtime was unavailable. The recorded first attempt could
not configure and its subsequent “No tests were found” is **not a pass**.
Reconfiguration with `-fsanitize=undefined -fsanitize-trap=all` needs no runtime;
that build and the actual glyph tests passed. See `ubsan.log` and
`ubsan-trap.log`. This checks exercised renderer paths, not all Qt or driver code.

## Native reproduction and cleanup

`tools/native-validation.py` launches only the sample, captures client buffers
with QQuickWindow::grabWindow and native windows with Spectacle, records geometry
with a temporary KWin script, and samples process CPU/RSS and whole-GPU telemetry.
It uses process-local Wayland, stderr logging and cache variables. Run from the
repository root with the local build paths above. Its HDMI setup/restoration
is specific to this owner's machine; **do not run display changes on another
machine without adapting and authorizing them**.

For this session the owner explicitly authorized enabling HDMI-A-1 for physical
4K, then restoring it. It was originally disabled, mode 69 (2560×1440), position
(-2560,0), scale 1, replication source DP-1. Initial enabling at mode 62 left it
mirrored: Qt correctly had no independent screen, so that attempted run failed.
The successful remaining run temporarily set `output.HDMI-A-1.mirror.none` as
well as mode 62 / position (4480,0). The command syntax was verified in
[KDE's doctor source](https://github.com/KDE/libkscreen/blob/master/src/doctor/doctor.cpp).
The script restores mirroring, mode, position and disabled state in `finally`.
Original and final KScreen JSON records are retained. No other monitor modes,
brightness or scaling preferences were changed.

The final helper refuses to overwrite existing captures/samples; choose a fresh
`--output-dir /tmp/phosphor-002-new-evidence` when reproducing. These guards were
added after the recorded run and do not change sampling or rendering.

Use `--skip-1080` only to resume after the completed 1080p runs; it appends samples.
Normal-size 12/16 pt pairs, no-glow and process-local DPR 1.5/resizing follow.
The fixture's bulk update is a synthetic row-number change, not actual terminal
scrollback. The update and idle logs include paint counts and frameSwapped counts.
Those counts include initialization/capture and do not measure latency or dropped
frames. Measurements begin only after the seven-second live capture exists.

All sample processes exit automatically. Temporary build/download/cache data
can be removed under `/tmp/phosphor-002` after preserving required evidence;
no uninstaller or system restoration is needed. Keep repository source,
approved artwork and both research evidence directories. The first diagnostic
launch used Qt's default application cache; subsequent runs use the temporary
cache explicitly. This is not an application settings/default-terminal change.
