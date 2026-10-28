#!/bin/sh
# Fetch Godot at the pinned tag (GODOT_VERSION) into build/godot and apply patches/. Safe to re-run.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TAG=$(cat "$ROOT/GODOT_VERSION")
DEST="$ROOT/build/godot"

if [ ! -d "$DEST/.git" ]; then
	mkdir -p "$ROOT/build"
	git clone --depth 1 --branch "$TAG" https://github.com/godotengine/godot.git "$DEST"
fi

if [ ! -f "$DEST/.switch-patched" ]; then
	for p in "$ROOT"/patches/*.patch; do
		echo "applying $(basename "$p")"
		git -C "$DEST" apply "$p"
	done
	touch "$DEST/.switch-patched"
fi
