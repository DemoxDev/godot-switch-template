#!/bin/sh
# One command from a Godot project to an NRO: exports the pack headless with your editor, then packs it.
#
#   scripts/export-nro.sh <project_dir> --out game.nro [--preset NAME] [--name N] [--author A]
#                         [--version V] [--icon PATH]
#
# Defaults: preset = first one in export_presets.cfg (any platform works, a pack is platform-independent);
# name/icon = the project's config/name and config/icon; version = latest git tag of the project, or 1.0.0.
# GODOT=/path/to/godot selects the editor binary; it must be the same version as the engine in GODOT_VERSION.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
GODOT=${GODOT:-godot}

[ $# -ge 1 ] || { echo "usage: $0 <project_dir> --out game.nro [options]" >&2; exit 2; }
PROJECT=$(cd "$1" && pwd)
shift

OUT=""
PRESET=""
NAME=""
AUTHOR="Unknown"
VERSION=""
ICON=""
while [ $# -gt 0 ]; do
	case "$1" in
		--out) OUT=$2 ;;
		--preset) PRESET=$2 ;;
		--name) NAME=$2 ;;
		--author) AUTHOR=$2 ;;
		--version) VERSION=$2 ;;
		--icon) ICON=$2 ;;
		*) echo "unknown option: $1" >&2; exit 2 ;;
	esac
	shift 2
done
[ -n "$OUT" ] || { echo "--out is required" >&2; exit 2; }

if [ -z "$PRESET" ]; then
	[ -f "$PROJECT/export_presets.cfg" ] || { echo "no export_presets.cfg: add any export preset in the editor first" >&2; exit 2; }
	PRESET=$(grep -m1 '^name=' "$PROJECT/export_presets.cfg" | cut -d'"' -f2)
fi
[ -n "$NAME" ] || NAME=$(grep -m1 '^config/name=' "$PROJECT/project.godot" | cut -d'"' -f2)
if [ -z "$VERSION" ]; then
	VERSION=$(git -C "$PROJECT" describe --tags --abbrev=0 2>/dev/null | sed 's/^v//' || true)
	[ -n "$VERSION" ] || VERSION="1.0.0"
fi
if [ -z "$ICON" ]; then
	RES=$(grep -m1 '^config/icon=' "$PROJECT/project.godot" | cut -d'"' -f2 | sed 's|^res://||')
	[ -z "$RES" ] || ICON="$PROJECT/$RES"
fi

mkdir -p "$ROOT/build"
TMP=$(mktemp -d "$ROOT/build/export.XXXXXX")
trap 'rm -rf "$TMP"' EXIT

echo "exporting pack: preset '$PRESET'"
"$GODOT" --headless --path "$PROJECT" --export-pack "$PRESET" "$TMP/game.pck"

set -- --pck "$TMP/game.pck" --out "$OUT" --name "$NAME" --author "$AUTHOR" --version "$VERSION"
[ -z "$ICON" ] || set -- "$@" --icon "$ICON"
"$ROOT/scripts/pack-nro.sh" "$@"
