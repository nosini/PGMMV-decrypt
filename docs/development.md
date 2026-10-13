# Development

The decoder is a small static library (`src/`) built into the Linux player.
The player's sources live in a separate repository,
[nosini/PGMMV](https://github.com/nosini/PGMMV); a patch in `patches/` adds the
calls into the decoder.

## Building the player with the decoder

You need the player's build dependencies plus the Nettle development files.
Check out the player at the commit the patches are made for, apply them and
build with `PGMMV_DECRYPTION_DIR` pointing here:

```
git clone https://github.com/nosini/PGMMV.git pgmmv
git -C pgmmv checkout "$(cat PGMMV-decrypt/patches/base-commit)"
git -C pgmmv apply "$PWD"/PGMMV-decrypt/patches/*.patch
cmake -S pgmmv/Player -B build -G Ninja -DPGMMV_CONFIGURATION=Runtime \
    -DPGMMV_DECRYPTION_DIR="$PWD"/PGMMV-decrypt
cmake --build build
```

The player's sources mix CRLF and LF line endings. `git apply` keeps them;
`git am` needs `--keep-cr`. The player needs GLFW 3.4 or newer.

A patched player built without `PGMMV_DECRYPTION_DIR` refuses encrypted games
with "this player was built without support for encrypted games".

## The portable build

Released players are built in a Debian 12 container so that they run on
other distributions too. A program built against an older glibc runs on newer
ones, and the libraries whose versions differ between distributions are
bundled:

- [`ci/dependencies.txt`](../ci/dependencies.txt) lists the Debian packages.
- [`ci/build-glfw.sh`](../ci/build-glfw.sh) builds GLFW 3.5, which Debian 12
  doesn't have, with X11 and Wayland support.
- [`ci/build-player.sh`](../ci/build-player.sh) applies the patch, builds and
  tests the player, installs it into a folder, and packs it.
- Nettle is linked statically (`-DPGMMV_STATIC_NETTLE=ON`): a bundled
  `libnettle.so.8` would replace the system's own copy, which the system's
  GnuTLS may need in a newer version.
- [`ci/bundle-libs.py`](../ci/bundle-libs.py) copies every library the
  player needs into `lib/`, except those listed in
  [`ci/host-libraries.txt`](../ci/host-libraries.txt), and points the player
  at that folder with a relative run path. The listed libraries come from the
  system: glibc, the graphics drivers, X11 and Wayland, the sound servers,
  GTK 3, curl, OpenSSL and SQLite. The Debian copyright files of the bundled
  libraries go into `share/pgmmv-player/licenses/libraries/`.

The player loads libvlc itself when a game plays a movie, so VLC isn't
bundled either: without it, the player runs and skips movies.

To build the same tarball locally with Podman:

```
podman run --rm -v "$PWD":/work:Z -w /work debian:12 sh -c '
    apt-get update && apt-get install -y --no-install-recommends \
        $(grep -v "^#" PGMMV-decrypt/ci/dependencies.txt) &&
    sh PGMMV-decrypt/ci/build-glfw.sh &&
    sh PGMMV-decrypt/ci/build-player.sh PGMMV-decrypt pgmmv dist local'
```

## Updating the patch

Commit changes to the hook on top of the player in its own repository, then
export them and record the new base:

```
git -C pgmmv format-patch --zero-commit --no-signature -o "$PWD"/PGMMV-decrypt/patches BASE
git -C pgmmv rev-parse BASE > PGMMV-decrypt/patches/base-commit
```

where `BASE` is the player commit without the hook. Remove the old patch files
first.

## Tests

Building this directory on its own compiles the decoder and its tests, which
use synthetic vectors and no game assets:

```
cmake -S PGMMV-decrypt -B build-decrypt
cmake --build build-decrypt
ctest --test-dir build-decrypt --output-on-failure
```

When the player is built with the decoder, `ctest` in its build directory runs
these tests together with the player's own.

How the format works and how the decoder was checked against the official
player is described in [format.md](format.md).

## Continuous integration and releases

`.github/workflows/build.yml` runs in Debian 12 containers. The
decoder tests run on every push and pull request. The player build runs when
started by hand from the Actions tab, which leaves the tarball as a workflow
artifact, and for tags starting with `v`. A tag also creates a GitHub release
with the tarball attached; tags with a suffix, such as `v1.0-rc1`, become
prereleases.

```
git tag v1.0
git push origin v1.0
```
