#include "EncryptedResources.h"

// Adapted from blluv/pgmm_decrypt (MIT, Copyright (c) 2022 blluv).
// The license is preserved in licenses/pgmm_decrypt-LICENSE.txt.

#include <nettle/twofish.h>
#include <algorithm>

namespace pgmmv {
namespace {

// Format constant, shared by the metadata mask and resource CBC initialization.
constexpr std::array<uint8_t, 16> initialization = {{
    0xa0, 0x47, 0xe9, 0x3d, 0x23, 0x0a, 0x4c, 0x62,
    0xa7, 0x44, 0xb1, 0xa4, 0xee, 0x85, 0x7f, 0xba
}};

// PGMMV's short-key cipher, also used to wrap metadata keys. It has no
// Twofish key schedule: each block is a fixed permutation followed by CBC.
constexpr size_t weakSource[] = {11, 8, 9, 10, 13, 14, 15, 12,
                                 3, 0, 1, 2, 5, 6, 7, 4};

void decodeWeakBlocks(const uint8_t *data, size_t size, uint8_t *output)
{
    const uint8_t *previous = initialization.data();
    for (size_t offset = 0; offset < size; offset += 16) {
        for (size_t i = 0; i < 16; ++i)
            output[offset + i] = data[offset + weakSource[i]] ^ previous[i];
        previous = data + offset;
    }
}

int base64Value(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

}

bool decodeResourceKey(const uint8_t *encoded, size_t size, ResourceKey &key)
{
    key = ResourceKey{};
    if (!encoded || (size != 16 && size != 32)) return false;

    decodeWeakBlocks(encoded, size, key.bytes.data());
    key.size = size;
    key.available = true;
    return true;
}

const char *loadResourceKey(const char *base64, size_t length, ResourceKey &key)
{
    key = ResourceKey{};
    if (!base64) {
        key.available = true;
        return nullptr;
    }
    // Padded base64 of the 16- or 32-byte metadata.
    size_t digits;
    if (length == 24 && base64[22] == '=' && base64[23] == '=') digits = 22;
    else if (length == 44 && base64[43] == '=') digits = 43;
    else return "unsupported encrypted game key in data/info.json (16 or 32 bytes required)";

    uint8_t metadata[32];
    size_t size = 0;
    unsigned bits = 0, pending = 0;
    for (size_t i = 0; i < digits; ++i) {
        const int value = base64Value(base64[i]);
        if (value < 0) return "invalid base64 key in data/info.json";
        bits = bits << 6 | value;
        pending += 6;
        if (pending >= 8) {
            pending -= 8;
            metadata[size++] = uint8_t(bits >> pending);
        }
    }
    if (!decodeResourceKey(metadata, size, key))
        return "invalid encrypted game key in data/info.json";
    return nullptr;
}

ResourceStatus decodeResource(const uint8_t *data, size_t size,
    const ResourceKey &key, std::vector<uint8_t> &output)
{
    if (size < 3 || !data || data[0] != 'e' || data[1] != 'n' || data[2] != 'c')
        return ResourceStatus::Plain;
    if (size < 4) return ResourceStatus::InvalidHeader;

    const size_t payloadSize = size - 4;
    const size_t padding = data[3];
    if (padding > 15 || payloadSize % 16 != 0 || padding > payloadSize)
        return ResourceStatus::InvalidHeader;
    if (!key.available) return ResourceStatus::MissingKey;
    if (key.size != 0 && key.size != 16 && key.size != 32)
        return ResourceStatus::MissingKey;

    const size_t originalSize = payloadSize - padding;
    // The native reader uses strlen/strncpy: the first NUL terminates the
    // password, and bytes after it never enter the file key.
    size_t effectiveSize = 0;
    while (effectiveSize < key.size && key.bytes[effectiveSize] != 0) ++effectiveSize;
    if (effectiveSize <= 8) {
        std::vector<uint8_t> decoded(payloadSize);
        decodeWeakBlocks(data + 4, payloadSize, decoded.data());
        decoded.resize(originalSize);
        output.swap(decoded);
        return ResourceStatus::Decoded;
    }

    std::array<uint8_t, 32> fileKey{};
    std::copy_n(key.bytes.begin(), effectiveSize, fileKey.begin());
    // XOR the little-endian original length into only its significant bytes.
    // A zero resulting key byte is represented by one in this format.
    size_t remaining = originalSize;
    for (size_t i = 0; remaining != 0; ++i, remaining >>= 8)
        fileKey[i] = std::max<uint8_t>(1, fileKey[i] ^ (remaining & 255));

    // The original player rounds the meaningful password length to a multiple
    // of eight bytes: 9..16 -> 128 bits, 17..24 -> 192, 25..32 -> 256.
    const size_t cipherKeySize = (effectiveSize + 7) & ~size_t(7);
    twofish_ctx cipher;
    twofish_set_key(&cipher, cipherKeySize, fileKey.data());
    std::vector<uint8_t> decoded(payloadSize);
    if (payloadSize) twofish_decrypt(&cipher, payloadSize, decoded.data(), data + 4);
    for (size_t offset = 0; offset < payloadSize; offset += 16) {
        const uint8_t *previous = offset ? data + 4 + offset - 16 : initialization.data();
        for (size_t i = 0; i < 16; ++i) decoded[offset + i] ^= previous[i];
    }
    decoded.resize(originalSize);
    output.swap(decoded);
    return ResourceStatus::Decoded;
}

}
