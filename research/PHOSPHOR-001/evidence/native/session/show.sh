#!/usr/bin/env bash
cd /home/Nozba/Codex/Project-PHOSPHOR/Project-PHOSPHOR/prototypes/ghostty-molten-amber
python3 -B fixtures.py static
fastfetch --config none --logo none --structure OS:Kernel:Shell:Display:DE:WM:Terminal:CPU:GPU:Memory
export PS1='PHOSPHOR $ '
exec /bin/bash --noprofile --norc
