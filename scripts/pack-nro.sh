#!/bin/sh
# Turn an exported Godot pack into an installable homebrew NRO using the Switch engine binary.
#
#   scripts/pack-nro.sh --pck game.pck --out game.nro [--name "My Game"] [--author "Me"]
#                       [--version 1.0.0] [--icon icon.png] [--template out/godot.switch.template_release.arm64]
#
# The icon is resized to the 256x256 JPEG the NRO format requires (a placeholder is used without --icon).
# Copy the result to sdmc:/switch/ on the SD card and start it from the Homebrew Menu.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
NAME="Godot Game"
AUTHOR="Unknown"
VERSION="1.0.0"
ICON=""
PCK=""
OUT=""
TEMPLATE="$ROOT/out/godot.switch.template_release.arm64"

while [ $# -gt 0 ]; do
	case "$1" in
		--pck) PCK=$2 ;;
		--out) OUT=$2 ;;
		--name) NAME=$2 ;;
		--author) AUTHOR=$2 ;;
		--version) VERSION=$2 ;;
		--icon) ICON=$2 ;;
		--template) TEMPLATE=$2 ;;
		*) echo "unknown option: $1" >&2; exit 2 ;;
	esac
	shift 2
done

[ -f "$PCK" ] || { echo "--pck: file not found: $PCK" >&2; exit 2; }
[ -n "$OUT" ] || { echo "--out is required" >&2; exit 2; }
[ -f "$TEMPLATE" ] || { echo "engine binary not found: $TEMPLATE (run scripts/build.sh first)" >&2; exit 2; }
[ -z "$ICON" ] || [ -f "$ICON" ] || { echo "--icon: file not found: $ICON" >&2; exit 2; }

docker build -q -t godot-switch-build - < "$ROOT/Dockerfile" > /dev/null

mkdir -p "$ROOT/build"
WORK=$(mktemp -d "$ROOT/build/pack.XXXXXX")
trap 'rm -rf "$WORK"' EXIT

mkdir "$WORK/romfs"
cp "$PCK" "$WORK/romfs/game.pck"
# SD folder name: sdmc:/switch/<name with unsafe characters replaced>. Read by the engine at startup.
printf '%s\n' "$NAME" | tr -c 'A-Za-z0-9._\n-' '_' > "$WORK/romfs/app.id"
cp "$TEMPLATE" "$WORK/engine.elf"
[ -z "$ICON" ] || cp "$ICON" "$WORK/icon.src"

docker run --rm --user "$(id -u):$(id -g)" -v "$WORK:/w" -w /w \
	-e NAME="$NAME" -e AUTHOR="$AUTHOR" -e VERSION="$VERSION" godot-switch-build sh -eu -c '
	if [ -f icon.src ]; then
		convert icon.src -background black -alpha remove -alpha off -resize 256x256 -gravity center -extent 256x256 -quality 92 icon.jpg
	else
		convert -size 256x256 "gradient:#478cbf-#1b2a3a" -quality 92 icon.jpg
	fi
	/opt/devkitpro/tools/bin/nacptool --create "$NAME" "$AUTHOR" "$VERSION" app.nacp
	/opt/devkitpro/tools/bin/elf2nro engine.elf out.nro --icon=icon.jpg --nacp=app.nacp --romfsdir=romfs
'

mkdir -p "$(dirname "$OUT")"
cp "$WORK/out.nro" "$OUT"
echo "packed: $OUT"
