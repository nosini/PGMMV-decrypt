# Development

The decoder is a small static library (`src/`) built into the Linux player.
The player's sources live in a separate repository,
[nosini/PGMMV](https://github.com/nosini/PGMMV); a patch in `patches/` adds the
calls into the decoder.

## Building the player with the decoder

You need the player's build dependencies plus the Nettle development files.
On openSUSE Tumbleweed, [`ci/dependencies.txt`](../ci/dependencies.txt) lists
every package:

```
sudo zypper install $(grep -v '^#' ci/dependencies.txt)
```

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
`git am` needs `--keep-cr`. [`ci/build-player.sh`](../ci/build-player.sh) does
the patch, build, test and packaging steps in one go.

A patched player built without `PGMMV_DECRYPTION_DIR` refuses encrypted games
with "this player was built without support for encrypted games".

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

`.github/workflows/build.yml` runs in an openSUSE Tumbleweed container. The
decoder tests run on every push and pull request. The player build runs when
started by hand from the Actions tab, which leaves the tarball as a workflow
artifact, and for tags starting with `v`. A tag also creates a GitHub release
with the tarball attached; tags with a suffix, such as `v1.0-rc1`, become
prereleases.

```
git tag v1.0
git push origin v1.0
```
