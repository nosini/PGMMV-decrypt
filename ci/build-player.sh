#!/bin/sh
# Builds the portable Linux player with this decoder and packs it.
#
# usage: ci/build-player.sh DECRYPT_DIR PLAYER_DIR OUTPUT_DIR VERSION
#
# PLAYER_DIR is a checkout of the Linux player at patches/base-commit. The
# patches are applied to it in place; the build happens in ./build. Meant for
# the Debian 12 container of the CI workflow, after ci/build-glfw.sh.
set -eu

decrypt=$(realpath "$1")
player=$(realpath "$2")
mkdir -p "$3"
output=$(realpath "$3")
name=pgmmv-player-decrypt-$4-x86_64
stage=stage/$name

# The player sources mix CRLF and LF line endings; git apply keeps them.
git -C "$player" apply "$decrypt"/patches/*.patch

cmake -S "$player/Player" -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DPGMMV_CONFIGURATION=Runtime \
    -DPGMMV_DECRYPTION_DIR="$decrypt" -DPGMMV_STATIC_NETTLE=ON
cmake --build build
ctest --test-dir build --output-on-failure

rm -rf "$stage"
cmake --install build --prefix "$stage"
# The player is installed as a plain program, which --strip leaves alone.
strip "$stage/bin/pgmmv-player"
licenses=$stage/share/pgmmv-player/licenses
cp "$player/LICENSE" "$licenses/PGMMV-LICENSE.txt"
python3 "$decrypt/ci/bundle-libs.py" "$stage/bin/pgmmv-player" "$stage/lib" "$licenses/libraries"
cp glfw-src/LICENSE.md "$licenses/libraries/glfw-LICENSE.md"

tar -C stage -czf "$output/$name.tar.gz" "$name"
echo "$output/$name.tar.gz"
