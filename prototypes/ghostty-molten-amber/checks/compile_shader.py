#!/usr/bin/env python3
"""Compile with an existing libshaderc and the pinned upstream Ghostty prefix.

This checks GLSL to SPIR-V only, not Ghostty's SPIRV-Cross/driver pipeline,
rendering, terminal integration, or performance. It installs nothing.
"""
import argparse
import ctypes as c
import ctypes.util
import hashlib
from pathlib import Path

PREFIX_SHA256 = "8cb58b8bc941a1b3c9951e6102d61faf8f27080806e46572a97efdce4001a254"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("prefix", type=Path, help="Ghostty v1.4.0 src/renderer/shaders/shadertoy_prefix.glsl")
    args = parser.parse_args()
    prefix = args.prefix.read_bytes()
    if hashlib.sha256(prefix).hexdigest() != PREFIX_SHA256:
        parser.error("prefix does not match the inspected Ghostty v1.4.0 source")
    library = ctypes.util.find_library("shaderc_shared")
    if not library:
        parser.error("libshaderc_shared is unavailable; no compiler test was run")
    lib = c.CDLL(library)

    def bind(name, result, *types):
        function = getattr(lib, "shaderc_" + name)
        function.restype = result
        function.argtypes = list(types)
        return function

    compiler_init = bind("compiler_initialize", c.c_void_p)
    compiler_release = bind("compiler_release", None, c.c_void_p)
    options_init = bind("compile_options_initialize", c.c_void_p)
    options_release = bind("compile_options_release", None, c.c_void_p)
    target_env = bind("compile_options_set_target_env", None, c.c_void_p, c.c_int, c.c_uint)
    target_spirv = bind("compile_options_set_target_spirv", None, c.c_void_p, c.c_int)
    compile_spv = bind("compile_into_spv", c.c_void_p, c.c_void_p, c.c_char_p,
                       c.c_size_t, c.c_int, c.c_char_p, c.c_char_p, c.c_void_p)
    status = bind("result_get_compilation_status", c.c_int, c.c_void_p)
    message = bind("result_get_error_message", c.c_char_p, c.c_void_p)
    length = bind("result_get_length", c.c_size_t, c.c_void_p)
    release = bind("result_release", None, c.c_void_p)
    compiler, options = compiler_init(), options_init()
    if not compiler or not options:
        raise RuntimeError("shaderc initialization failed")
    try:
        # Same input target as Ghostty shadertoy.zig: Vulkan 1.2 / SPIR-V 1.5.
        # Fragment shader = 1; these enum values come from shaderc's public C API.
        target_env(options, 0, (1 << 22) | (2 << 12))
        target_spirv(options, 0x010500)
        shader = Path(__file__).resolve().parents[1] / "molten-amber.glsl"
        source = prefix + b"\n\n" + shader.read_bytes()
        for negative in (False, True):
            candidate = source + (b"\ninvalid syntax;\n" if negative else b"")
            result = compile_spv(compiler, candidate, len(candidate), 1,
                                 b"molten-amber.glsl", b"main", options)
            if not result:
                raise RuntimeError("shaderc returned no compilation result")
            try:
                code = status(result)
                if negative:
                    if code == 0:
                        raise RuntimeError("negative control unexpectedly compiled")
                    print("PASS: invalid GLSL rejected (negative control)")
                else:
                    if code != 0:
                        raise RuntimeError(message(result).decode("utf-8", errors="replace"))
                    print(f"PASS: pinned prefix + shader compiled to {length(result)} bytes of SPIR-V")
            finally:
                release(result)
    finally:
        options_release(options)
        compiler_release(compiler)


if __name__ == "__main__":
    main()
