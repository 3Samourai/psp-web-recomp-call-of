#!/usr/bin/env bash
# Prepares a fresh checkout: fetches PSPRecomp at the commit the patches were
# made against, applies them, links the web profile into it and installs the
# Emscripten SDK.  Safe to run again.
#   scripts/setup.sh [--no-emsdk]
set -euo pipefail
source "$(dirname "$0")/common.sh"

PSPRECOMP_URL="https://github.com/jessicanataliagta/PSPRecomp.git"
PSPRECOMP_COMMIT="f6e7d41"
EMSDK_VERSION="6.0.11"

if [ ! -d "$FRAMEWORK/.git" ]; then
  echo "== fetching PSPRecomp"
  git clone "$PSPRECOMP_URL" "$FRAMEWORK"
  git -C "$FRAMEWORK" checkout -q -b web "$PSPRECOMP_COMMIT"
  git -C "$FRAMEWORK" -c user.name=setup -c user.email=setup@localhost am -q "$ROOT"/patches/*.patch
fi

if [ ! -e "$FRAMEWORK/profiles/web" ]; then
  ln -s ../../profile "$FRAMEWORK/profiles/web"
  grep -qx "profiles/web" "$FRAMEWORK/.git/info/exclude" 2>/dev/null || echo "profiles/web" >> "$FRAMEWORK/.git/info/exclude"
fi

if [ "${1:-}" != "--no-emsdk" ] && [ ! -f "$EMSDK/emsdk_env.sh" ]; then
  echo "== installing the Emscripten SDK $EMSDK_VERSION"
  mkdir -p "$(dirname "$EMSDK")"
  git clone https://github.com/emscripten-core/emsdk.git "$EMSDK"
  "$EMSDK/emsdk" install "$EMSDK_VERSION"
  "$EMSDK/emsdk" activate "$EMSDK_VERSION"
fi

"$(dirname "$0")/build_tools.sh"
echo "== ready"
