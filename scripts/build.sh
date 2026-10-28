#!/bin/sh
# Build the Switch engine binary (an ELF) inside the toolchain container.
#   TARGET=template_release|template_debug  (default template_release; only release is tested)
#   JOBS=N                                  (default: all cores)
# Extra arguments go to scons, e.g. `scripts/build.sh module_gltf_enabled=yes`.
# Output: out/godot.switch.<target>.arm64
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TARGET=${TARGET:-template_release}
JOBS=${JOBS:-$(nproc)}
DEST="$ROOT/build/godot"

"$ROOT/scripts/setup.sh"
docker build -q -t godot-switch-build - < "$ROOT/Dockerfile" > /dev/null

# The platform is developed outside the Godot tree and copied in on every build.
mkdir -p "$DEST/platform/switch"
cp -a "$ROOT/platform/switch/." "$DEST/platform/switch/"

docker run --rm --user "$(id -u):$(id -g)" -v "$DEST:/src" -w /src godot-switch-build \
	scons platform=switch target="$TARGET" -j"$JOBS" "$@"

mkdir -p "$ROOT/out"
cp "$DEST/bin/godot.switch.$TARGET.arm64" "$ROOT/out/"
echo "built: $ROOT/out/godot.switch.$TARGET.arm64"
