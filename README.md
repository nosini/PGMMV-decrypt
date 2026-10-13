# PGMMV-decrypt

Play encrypted Pixel Game Maker MV games natively on Linux.

Pixel Game Maker MV games ship with a Windows player. The
[Linux build of the open-source player](https://github.com/nosini/PGMMV) runs
them natively, but it can't read games whose files were encrypted on export,
which most released games are. This project adds the missing decoder and
provides the Linux player with it built in. The player decrypts the game's
files in memory as it loads them, so the encrypted files on disk stay as they
are.

## Installation

Download `pgmmv-player-decrypt-VERSION-x86_64.tar.gz` from the
[latest release](../../releases/latest) and unpack it wherever you like to keep
programs, for example:

```
mkdir -p ~/.local/opt
tar -xzf pgmmv-player-decrypt-*-x86_64.tar.gz -C ~/.local/opt
```

The player is built on openSUSE Tumbleweed. On Tumbleweed, install the
libraries it needs that a desktop may not have yet:

```
sudo zypper install libglfw3 libGLEW2_3 libGLU1 libopenal1 libvlc5 libnettle9
```

Games that play videos also need VLC's plugins (`vlc-noX`). Other distributions
work if they provide equally recent versions of these libraries; Nettle 4
(`libnettle.so.9`) is the one older releases lack.

## Playing a game

Point the player at the game's folder, the one that contains the Windows
`player.exe`:

```
~/.local/opt/pgmmv-player-decrypt-*/bin/pgmmv-player --game-dir="/path/to/game"
```

Unencrypted games and editor projects work the same way. Keyboard and gamepad
controls are the ones the game defines.

Saves go to `~/.local/share/pgmmv-player/<game folder name>/save/`, unless the
game is set up to save inside its own folder. Like on Windows, the player also
writes a log, `action_log.txt`, into the game folder.

## When a game doesn't start

The player prints the reason on the terminal it was started from. Messages
about the encryption start with `player:`, for example:

- `cannot decode data/project.json (invalid key or unsupported encryption)`:
  the key in the game's `data/info.json` doesn't match its files, or the game
  uses an encryption this decoder doesn't know.
- `unsupported encrypted game key in data/info.json`: the key has a size the
  decoder doesn't support.

Games that depend on Windows DLL plugins (`plugins/win32/*.dll`) can't load
those plugins on Linux.

## Supported encryption

Both key formats that games store in `data/info.json` (16 and 32 bytes), files
encrypted with Twofish-128, -192 or -256, and the older weak cipher of games
exported without a key. How the format works is described in
[docs/format.md](docs/format.md).

## Building from source

See [docs/development.md](docs/development.md) for building the player with the
decoder, the patch it applies to the player, the tests and how releases are
made.

## License

MIT, see [LICENSE](LICENSE). The decoding algorithms are adapted from
[pgmm_decrypt](https://github.com/blluv/pgmm_decrypt) by blluv, also MIT
([licenses/pgmm_decrypt-LICENSE.txt](licenses/pgmm_decrypt-LICENSE.txt)). The
player itself is MIT-licensed by Gotcha Gotcha Games; all three licenses are
included in the release.
