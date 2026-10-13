# Encrypted resource format

This describes how Pixel Game Maker MV encrypts game resources and how this
decoder reads them. The key and resource algorithms are adapted from the
MIT-licensed
[pgmm_decrypt](https://github.com/blluv/pgmm_decrypt/tree/391767a14d68f6c5841b22d7c3eea4673a84bd54)
(Copyright (c) 2022 blluv; full license in
[licenses/pgmm_decrypt-LICENSE.txt](../licenses/pgmm_decrypt-LICENSE.txt)).
System [Nettle](https://www.lysator.liu.se/~nisse/nettle/) supplies the
standard Twofish implementation, including its 192-bit key mode.

## Format

An encrypted file starts with `enc` and a padding-count byte in 0–15. The rest
contains complete 16-byte cipher blocks. The original length excludes the
four-byte header and the stated padding. Ordinary files pass through unchanged.
The fixed initialization vector is:

```
a0 47 e9 3d 23 0a 4c 62 a7 44 b1 a4 ee 85 7f ba
```

The weak cipher applies a byte permutation to each block and XORs it with the
previous ciphertext block (the IV for the first block). Interpret each block
as four little-endian words: rotate words 0 and 2 left by eight bits, words 1
and 3 right by eight bits, then reorder them as 2,3,0,1.

`info.json` stores the game key in base64, encrypted with this weak CBC cipher.
A 32-byte value requires chaining the second block to the first ciphertext
block. The first NUL terminates the key; any later bytes are ignored. Copy
that prefix into a zero-padded file key to obtain the meaningful key length.
Lengths up to eight bytes use the weak resource cipher; lengths 9–16, 17–24
and 25–32 select Twofish-128, Twofish-192 and Twofish-256 respectively.
Builds without key metadata are tried with the legacy cipher, and
the decoded project must parse before loading continues.

For Twofish resources, XOR the significant little-endian bytes of the original
file length into the corresponding leading bytes of the game key, replacing
each resulting zero with one. Decrypt with that file key in CBC mode using the
fixed IV, then remove the stated padding.

## Validation

Static analysis of the official Live Development 2019 Windows player with
[REA](https://github.com/morluto/rea) and Ghidra confirmed metadata decoding,
the IV, file-length key derivation and rounding the key width to eight bytes.
This implementation uses the MIT decoder as its reference; it is not a
source-blind clean-room implementation.

Synthetic reference vectors in `tests/` cover both metadata lengths, the base64
key in `info.json`, second-block chaining, Twofish-128/192/256, key termination
with nonzero trailing bytes, binary plaintext, the eight/nine-byte cipher
boundary, short padded keys, keyless resources, padding and malformed headers.
The tests contain no game assets.

Beyond the tests, the decoder was checked against official sample games:
decoded project files parse, decoded PNGs have valid chunk CRCs, and bitmap
fonts and a tile image match the plain originals from the MIT player sources
byte for byte. Copies of an unencrypted game, re-encrypted in each mode, play
the same as the original. Games exported by a publisher with 32-byte key
metadata have not been available for testing.
