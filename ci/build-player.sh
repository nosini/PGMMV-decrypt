#!/bin/sh
# Builds the Linux player with this decoder and packs it into a tarball.
#
# usage: ci/build-player.sh DECRYPT_DIR PLAYER_DIR OUTPUT_DIR VERSION
#
# PLAYER_DIR is a checkout of the Linux player at patches/base-commit. The
# patches are applied to it in place; the build happens in ./build.
set -eu

decrypt=$(realpath "$1")
player=$(realpath "$2")
mkdir -p "$3"
output=$(realpath "$3")
name=pgmmv-player-decrypt-$4-x86_64

# The player sources mix CRLF and LF line endings; git apply keeps them.
git -C "$player" apply "$decrypt"/patches/*.patch

cmake -S "$player/Player" -B build -G Ninja -DPGMMV_CONFIGURATION=Runtime \
    -DPGMMV_DECRYPTION_DIR="$decrypt"
cmake --build build
ctest --test-dir build --output-on-failure

rm -rf "stage/$name"
cmake --install build --prefix "stage/$name"
# The player is installed as a plain program, which --strip leaves alone.
strip "stage/$name/bin/pgmmv-player"
cp "$player/LICENSE" "stage/$name/share/pgmmv-player/licenses/PGMMV-LICENSE.txt"
tar -C stage -czf "$output/$name.tar.gz" "$name"
echo "$output/$name.tar.gz"
