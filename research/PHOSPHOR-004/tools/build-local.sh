#!/usr/bin/env bash
# Reproduce this machine's local-only toolchain, without installing packages.
set -euo pipefail
kind=${1:-clang}
root=$(cd -- "$(dirname -- "$0")/../../.." && pwd)
qt_prefix=/tmp/phosphor-002/sysroot/usr
export LD_LIBRARY_PATH="$qt_prefix/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
flags="-L$qt_prefix/lib64 -isystem $qt_prefix/include/c++/16 -isystem $qt_prefix/include/c++/16/x86_64-redhat-linux -isystem $qt_prefix/include/c++/16/backward"
case "$kind" in
 clang) compiler=/usr/lib64/rocm/llvm/bin/clang++ ;;
 ubsan) compiler=/usr/lib64/rocm/llvm/bin/clang++; flags="-fsanitize=undefined -fsanitize-trap=all $flags" ;;
 gcc) compiler="$qt_prefix/bin/g++"; flags="-B$qt_prefix/libexec/gcc/x86_64-redhat-linux/16/ -B/usr/libexec/gcc/x86_64-redhat-linux/16/ -B/usr/lib/gcc/x86_64-redhat-linux/16/ $flags" ;;
 *) echo 'Expected clang, gcc or ubsan' >&2; exit 2 ;;
esac
"$qt_prefix/bin/cmake" -S "$root/prototypes/qt-terminal" -B "/tmp/phosphor-004/$kind-build" -G Ninja \
 -DLIBVTERM_SOURCE_DIR=/tmp/phosphor-004/libvterm-0.3.3 -DCMAKE_C_COMPILER=/usr/lib64/rocm/llvm/bin/clang -DCMAKE_C_FLAGS="${flags}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="$compiler" \
 -DCMAKE_PREFIX_PATH="$qt_prefix;/tmp/phosphor-001-native/sysroot/usr" -DCMAKE_CXX_FLAGS="$flags"
"$qt_prefix/bin/cmake" --build "/tmp/phosphor-004/$kind-build" -j3
QT_FORCE_STDERR_LOGGING=1 "$qt_prefix/bin/ctest" --test-dir "/tmp/phosphor-004/$kind-build" --output-on-failure
