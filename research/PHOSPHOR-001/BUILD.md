# Isolated native build provenance

On 2026-11-01 the owner authorized native validation, preferring an official
Fedora package and otherwise a local build of verified upstream Ghostty 1.4.0.

## Package check

Both the cached and refreshed official `fedora` / `updates` repositories
returned no Ghostty package:

```bash
dnf --refresh repoquery --available --repo=fedora --repo=updates \
  --queryformat '%{name} %{version}-%{release} %{repoid}' ghostty
```

No COPR, Terra, other repository, or community binary was enabled or used.
No RPM was installed and no administrator command was run. Build headers and
the missing layer-shell library were downloaded as Fedora RPMs, their RPM
signatures/digests checked, and their payloads extracted only under
`/tmp/phosphor-001-native/sysroot`. Installed runtime libraries were referenced
by local symlinks where a development package supplied only the linker name.
The dependency solver also downloaded unused multilib/weak packages; those
were not installed or added to the build root. The extracted packages are
recorded in [extracted-rpms.sha256](evidence/native/extracted-rpms.sha256).

## Upstream verification

- Source: `https://release.files.ghostty.org/1.4.0/ghostty-1.4.0.tar.gz`.
- Source SHA256: `cb188ac6c3a8278d5fd7bfef15d8c017104c87f638b5560eb38024a71ae0f005`.
- The accompanying [Minisign signature](evidence/native/ghostty-1.4.0.tar.gz.minisig)
  and its trusted comment both verified against the public key published in
  the [official build instructions](https://ghostty.org/docs/install/build).
  Existing PyNaCl verified Ed25519 signatures, including the `ED` format's
  Blake2b prehash, key ID, and trusted-comment signature. Nothing was installed
  for verification.
- The archive's `VERSION` is `1.4.0`; its custom-shader prefix matches the
  inspected tag commit `83fb95624b0439ee660c2679b43b38c8b63769cd` exactly.
- Zig 0.17.0 was downloaded from `ziglang.org`, and its SHA256 verified against
  the official [download index](https://ziglang.org/download/index.json).
  The selected URL/hash are preserved in [zig-0.17.0.json](evidence/native/zig-0.17.0.json).

The machine-readable [provenance record](evidence/native/build-provenance.json)
includes the upstream key and final local executable hash. The downloaded
archives, compiler, source, build caches, and executable remain under `/tmp`;
they are not redistributed in this PR. Preserve needed records before cleanup.

## Build

The successful final command, run from the verified source archive directory:

```bash
ZIG_GLOBAL_CACHE_DIR=/tmp/phosphor-001-native/zig-cache \
PKG_CONFIG_PATH=/tmp/phosphor-001-native/sysroot/usr/lib64/pkgconfig:/tmp/phosphor-001-native/sysroot/usr/share/pkgconfig \
PKG_CONFIG_SYSROOT_DIR=/tmp/phosphor-001-native/sysroot \
/tmp/phosphor-001-native/zig-x86_64-linux-0.17.0/zig build \
  -Doptimize=ReleaseFast -Dcpu=baseline -Dversion-string=1.4.0 \
  --prefix /tmp/phosphor-001-native/install -j8
```

`-Dversion-string=1.4.0` is an upstream build option: without it, this tarball
build initially labeled itself `1.4.0-dev+0000000` because no Git tag metadata
was present. That first preflight is excluded from final results. The final
[version output](evidence/native/ghostty-version.txt) confirms stable 1.4.0,
Zig 0.17.0, optimized build, GTK 4.22.5, libadwaita 1.9.4, and OpenGL renderer.
Build attempts/logs are retained, including the missing protocol/EGL development
files resolved locally. No upstream renderer source was changed.

Native launches used only process-local variables:

```bash
GDK_BACKEND=wayland \
LD_LIBRARY_PATH=/tmp/phosphor-001-native/sysroot/usr/lib64 \
GHOSTTY=/tmp/phosphor-001-native/install/bin/ghostty \
bash prototypes/ghostty-molten-amber/run.sh prototype
```

Logs confirm NVIDIA EGL and OpenGL 4.3, the normal 12 pt Noto Sans Mono font,
scale 1, and successful custom-shader loading. No CPU presentation fallback
or shader error was reported in the captured runs; absence of a warning is
not a separate DMABUF conformance test.

## Isolation correction found during native testing

The original launcher disabled loading default files, but Ghostty creates a
template **before** parsing that flag. The first preflight therefore created
`~/.config/ghostty/config.ghostty`. It was identified by the creation log and
its exact template contents, preserved in the
[original session archive](evidence/native/session/initial-user-template.ghostty), then
removed after verifying the preserved bytes. No pre-existing config was
overwritten. Its hash is in the provenance record.

The launcher now creates a private temporary XDG config directory, waits for
Ghostty, and removes only that directory on exit. Native logs show subsequent
templates inside this private directory; the user's config file stays absent.
An added regression check verifies early template writes and cleanup while
preserving an existing config sentinel. Konsole, the default terminal, system
terminal settings, and physical display modes were not changed.
