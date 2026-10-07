#!/usr/bin/env bash
# Translates a decrypted PSP module to C++: scripts/generate.sh <game> <module.prx|EBOOT.ELF>
set -euo pipefail
source "$(dirname "$0")/common.sh"
game="$1"; module="$2"
out="$FRAMEWORK/profiles/web/generated/$game"
[ -x "$TOOLS_BUILD/psp_recomp" ] || "$(dirname "$0")/build_tools.sh"
mkdir -p "$out"  # psp_recomp only rewrites units whose code changed
"$TOOLS_BUILD/psp_recomp" "$module" --auto "$out" 0x08804000 0x4000
