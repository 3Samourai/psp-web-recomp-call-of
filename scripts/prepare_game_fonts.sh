#!/usr/bin/env bash
# Bundle original system fonts for games importing sceLibFont.
set -euo pipefail
source "$(dirname "$0")/common.sh"
name="$1"
game="$ROOT/games/$name"
registry="$ROOT/profile/generated/$name/generated_registry.cpp"
python3 -I -c 'import pathlib,sys; sys.exit(0 if "sceLibFont" in pathlib.Path(sys.argv[1]).read_text() else 1)' "$registry" || exit 0

fonts="${PSP_FONT_DIR:-$game/firmware/F0/font}"
if [ ! -f "$fonts/ltn8.pgf" ]; then
  updater="$game/root/disc/PSP_GAME/SYSDIR/UPDATE/DATA.BIN"
  decrypt="${PSP_PSAR_DECRYPT:-$ROOT/tools/pspdecrypt/pspdecrypt}"
  [ -f "$updater" ] || { echo "System fonts required: set PSP_FONT_DIR to an original flash0/font folder." >&2; exit 1; }
  [ -x "$decrypt" ] || { echo "Build John-K/pspdecrypt in tools/pspdecrypt or set PSP_PSAR_DECRYPT (see README.md)." >&2; exit 1; }
  "$decrypt" -O "$game/firmware" "$updater"
  fonts="$game/firmware/F0/font"
fi
python3 -I "$ROOT/scripts/prepare_fonts.py" "$fonts" "$game/preload/fonts"
[ -s "$game/preload/fonts/ltn8.pwf" ] || { echo "Missing original NewRodin Latin font." >&2; exit 1; }
