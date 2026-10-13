# pgmmv-linux-decryption

Lets the native Linux build of the Pixel Game Maker MV player run encrypted
games. Games exported with encryption turned on store their project data,
images and audio encrypted; the open-source player can't read them on its own.
With this decoder built in, the player decrypts those files in memory while the
game loads them. The game folder is never modified.

It supports both key formats that `data/info.json` can carry (16 and 32 bytes),
the Twofish-128, -192 and -256 ciphers, and the older weak cipher used by games
exported without a key.

## Building the player with it

You need the player's own build dependencies plus the Nettle development files
(`libnettle-devel` on openSUSE, `nettle-dev` on Debian and Ubuntu).

The player needs a small change to call the decoder. It is in `patches/`;
apply it to a checkout of the Linux player first. Its sources mix CRLF and LF
line endings, so pass `--keep-cr` to `git am`, or use `git apply`:

```
git -C pgmmv-linux am --keep-cr "$PWD"/pgmmv-linux-decryption/patches/*.patch
```

Then point the player's CMake build at this directory:

```
cmake -S pgmmv-linux/Player -B build -G Ninja -DPGMMV_CONFIGURATION=Runtime \
    -DPGMMV_DECRYPTION_DIR=$PWD/pgmmv-linux-decryption
cmake --build build
```

Run an encrypted game like any other:

```
build/player --game-dir=/path/to/game
```

If a game can't be decrypted, the player exits with a message on the terminal,
for example when `data/info.json` holds a key in an unknown format. A patched
player built without `PGMMV_DECRYPTION_DIR` refuses encrypted games with "this
player was built without support for encrypted games".

## Tests

Building this directory on its own compiles the decoder and its tests:

```
cmake -S pgmmv-linux-decryption -B build-decryption
cmake --build build-decryption
ctest --test-dir build-decryption --output-on-failure
```

How the format works and how the decoder was checked is described in
[docs/format.md](docs/format.md).

## License

MIT, see [LICENSE](LICENSE). The decoding algorithms are adapted from
[pgmm_decrypt](https://github.com/blluv/pgmm_decrypt) by blluv, also MIT
([licenses/pgmm_decrypt-LICENSE.txt](licenses/pgmm_decrypt-LICENSE.txt)).
Both license files are installed with the player.
