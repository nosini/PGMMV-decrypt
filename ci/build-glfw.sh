#!/bin/sh
# Builds GLFW into /usr/local. Debian 12 ships GLFW 3.3; the player needs 3.4
# or newer. It is bundled with the player, with X11 and Wayland support.
set -eu

version=3.5.1
commit=d9d6f0f1f967807ffade6598ea9a631ebaf37a56

git clone --quiet --depth 1 --branch "$version" https://github.com/glfw/glfw.git glfw-src
test "$(git -C glfw-src rev-parse HEAD)" = "$commit"
cmake -S glfw-src -B glfw-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF \
    -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_X11=ON -DGLFW_BUILD_WAYLAND=ON
cmake --build glfw-build
cmake --install glfw-build --prefix /usr/local
ldconfig
