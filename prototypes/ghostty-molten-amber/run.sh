#!/usr/bin/env bash
# No installation, default-config loading, or persistent configuration changes.
set -euo pipefail

usage() {
    printf 'Usage: %s baseline|prototype [-- COMMAND [ARG...]]\n' "$0" >&2
    exit 2
}

[[ $# -ge 1 ]] || usage
mode=$1
shift
case "$mode" in
    baseline|prototype) ;;
    *) usage ;;
esac
if [[ $# -gt 0 ]]; then
    [[ $1 == -- && $# -gt 1 ]] || usage
    shift
fi

prototype_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
ghostty_bin=${GHOSTTY:-ghostty}
if ! command -v -- "$ghostty_bin" >/dev/null 2>&1; then
    printf 'Ghostty is unavailable. Supply an owner-approved binary with GHOSTTY=/absolute/path.\n' >&2
    exit 127
fi

args=(
    --config-default-files=false
    "--config-file=$prototype_dir/config"
    --gtk-single-instance=false
    --class=org.phosphor.Spike001
    "--working-directory=$prototype_dir"
)
if [[ $mode == prototype ]]; then
    args+=("--custom-shader=$prototype_dir/molten-amber.glsl")
fi
if [[ $# -eq 0 ]]; then
    set -- /bin/bash --noprofile --norc
fi
# Keep this disposable shell out of the user's Bash history.
export HISTFILE=/dev/null
exec "$ghostty_bin" "${args[@]}" -e "$@"
