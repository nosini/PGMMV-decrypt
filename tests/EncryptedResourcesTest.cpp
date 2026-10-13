#include "EncryptedResources.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

static void require(bool value, const char *message)
{
    if (!value) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}

int main()
{
    // Synthetic vector, cross-checked with libgcrypt 1.10 through an independent
    // Python format probe. Includes binary plaintext and the length-XOR
    // zero-to-one rule; the encrypted game assets are not test fixtures.
    const uint8_t metadata[] = {0x43,0xb9,0xad,0xa1,0xb7,0xe4,0x8e,0x73,
                               0x45,0x16,0x3c,0xb1,0x67,0x21,0x09,0x48};
    const std::array<uint8_t,16> expectedKey = {{17,2,255,1,2,3,4,5,6,7,8,9,10,11,12,13}};
    const uint8_t encrypted[] = {
        'e','n','c',15,0x48,0xca,0x3c,0xbc,0xa1,0x65,0xd0,0x87,0xa8,0xc9,0xde,0xd5,
        0x8b,0xb6,0x9d,0x48,0xbf,0x4a,0x5f,0x0a,0xd7,0x8c,0x53,0xab,0x66,0xea,0x74,0x7e,
        0x30,0xca,0x17,0xb5
    };
    const uint8_t expected[] = {'P','G','M','M','V',0,'t','e','s','t','-','d','a','t','a','!',255};

    pgmmv::ResourceKey key;
    require(pgmmv::decodeResourceKey(metadata, sizeof(metadata), key), "metadata rejected");
    require(key.available && key.size == 16
        && std::equal(expectedKey.begin(), expectedKey.end(), key.bytes.begin()), "metadata permutation mismatch");
    const auto originalKey = key.bytes;
    std::vector<uint8_t> output;
    require(pgmmv::decodeResource(encrypted, sizeof(encrypted), key, output)
        == pgmmv::ResourceStatus::Decoded, "ciphertext rejected");
    require(output == std::vector<uint8_t>(expected, expected + sizeof(expected)), "plaintext mismatch");
    require(key.bytes == originalKey, "game key mutated during decoding");
    require(pgmmv::decodeResource(encrypted, sizeof(encrypted), key, output)
        == pgmmv::ResourceStatus::Decoded, "second decode failed");
    require(output.size() == sizeof(expected), "padding retained");

    auto unchanged = output;
    const uint8_t plain[] = {'{','}','\n'};
    require(pgmmv::decodeResource(plain, sizeof(plain), key, output)
        == pgmmv::ResourceStatus::Plain && output == unchanged, "plain input changed output");
    require(pgmmv::decodeResource(nullptr, 0, key, output)
        == pgmmv::ResourceStatus::Plain, "empty plain input rejected");
    require(pgmmv::decodeResource(encrypted, 3, key, output)
        == pgmmv::ResourceStatus::InvalidHeader, "truncated header accepted");
    require(pgmmv::decodeResource(encrypted, sizeof(encrypted)-1, key, output)
        == pgmmv::ResourceStatus::InvalidHeader, "partial block accepted");
    const uint8_t badPadding[] = {'e','n','c',16};
    require(pgmmv::decodeResource(badPadding, sizeof(badPadding), key, output)
        == pgmmv::ResourceStatus::InvalidHeader, "invalid padding accepted");
    const uint8_t oversizedPadding[] = {'e','n','c',1};
    require(pgmmv::decodeResource(oversizedPadding, sizeof(oversizedPadding), key, output)
        == pgmmv::ResourceStatus::InvalidHeader, "padding beyond payload accepted");
    require(output == unchanged, "failure changed output");
    require(!pgmmv::decodeResourceKey(metadata, 15, key) && !key.available,
        "short metadata retained a previous key");
    require(pgmmv::decodeResource(encrypted, sizeof(encrypted), key, output)
        == pgmmv::ResourceStatus::MissingKey, "encrypted input accepted without a key");
    const uint8_t invalidMetadata[48] = {};
    require(!pgmmv::decodeResourceKey(invalidMetadata, sizeof(invalidMetadata), key),
        "unsupported key format accepted");

    // Reference pgmm_decrypt's weak cipher wraps two metadata blocks; the
    // second block must chain to the first ciphertext, not the fixed IV.
    const uint8_t metadata32[] = {
        0x4e,0xba,0xa8,0xae,0xaa,0xe3,0x8b,0x70,0x45,0xea,0x39,0xa1,0x6a,0x26,0x0c,0x4b,
        0xf0,0x22,0xbd,0x5c,0x6b,0x77,0x38,0x13,0xa8,0xbb,0xba,0x5f,0x68,0xbf,0xf5,0x9c
    };
    const uint8_t encrypted32[] = {
        'e','n','c',12,0x8c,0x73,0x12,0xcb,0xf0,0x2a,0xe2,0xd8,0x86,0x03,0x84,0xee,
        0xf8,0xff,0x82,0xef,0x96,0x61,0x21,0x48,0x17,0xb7,0xd9,0xcc,0x5f,0xec,0x04,0xdc,
        0xa2,0xf0,0x28,0xe6
    };
    const uint8_t plaintext32[] = {'3','2','-','b','y','t','e',' ','P','G','M','M','V',' ','t','e','s','t',0,255};
    require(pgmmv::decodeResourceKey(metadata32, sizeof(metadata32), key) && key.size == 32,
        "32-byte metadata rejected");
    for (size_t i = 0; i < 32; ++i) require(key.bytes[i] == i + 1, "32-byte metadata chaining mismatch");
    require(pgmmv::decodeResource(encrypted32, sizeof(encrypted32), key, output)
        == pgmmv::ResourceStatus::Decoded, "Twofish256 resource rejected");
    require(output == std::vector<uint8_t>(plaintext32, plaintext32 + sizeof(plaintext32)),
        "Twofish256 plaintext mismatch");

    // A 17-byte meaningful key rounds to 192 bits even though its metadata
    // occupies two cipher blocks. Cross-checked against the published 192-bit
    // Twofish vector and the reference metadata wrapper.
    const uint8_t metadata192[] = {
        0x4e,0xba,0xa8,0xae,0xaa,0xe3,0x8b,0x70,0x45,0xea,0x39,0xa1,0x6a,0x26,0x0c,0x4b,
        0xea,0x39,0xa1,0x45,0x4b,0x6a,0x26,0x0c,0xba,0xa8,0xae,0x5f,0x70,0xaa,0xe3,0x8b
    };
    const uint8_t encrypted192[] = {
        0x65,0x6e,0x63,0x08,0xee,0x94,0x9f,0xd7,0xcb,0x00,0x65,0x0d,0xdd,0xdf,0xb9,0x09,
        0x51,0x42,0x31,0x9e,0x62,0x7d,0x89,0xc8,0x26,0x16,0xbb,0xb0,0xe3,0x52,0xdb,0xea,
        0x31,0x86,0x15,0xc5
    };
    const uint8_t plaintext192[] = {
        0x31,0x39,0x32,0x2d,0x62,0x69,0x74,0x20,0x50,0x47,0x4d,0x4d,0x56,0x20,0x72,0x65,
        0x73,0x6f,0x75,0x72,0x63,0x65,0x00,0xff
    };
    require(pgmmv::decodeResourceKey(metadata192, sizeof(metadata192), key),
        "192-bit key metadata rejected");
    require(pgmmv::decodeResource(encrypted192, sizeof(encrypted192), key, output)
        == pgmmv::ResourceStatus::Decoded, "Twofish192 resource rejected");
    require(output == std::vector<uint8_t>(plaintext192, plaintext192 + sizeof(plaintext192)),
        "Twofish192 plaintext mismatch");
    // The native player copies only the prefix before the first NUL. Later
    // bytes must affect neither the cipher width nor the padded cipher key.
    key.bytes[18] = 99; // Inside the rounded 24-byte cipher key.
    key.bytes[31] = 255; // Outside it, but inside the metadata.
    require(pgmmv::decodeResource(encrypted192, sizeof(encrypted192), key, output)
        == pgmmv::ResourceStatus::Decoded
        && output == std::vector<uint8_t>(plaintext192, plaintext192 + sizeof(plaintext192)),
        "bytes after a key terminator affected Twofish192");

    const uint8_t weakResource[] = {
        'e','n','c',13,0x34,0xd9,0xc1,0xce,0xdf,0x9c,0xa5,0x0b,0x22,0x8e,0x5c,0xcc,
        0x01,0x40,0x73,0x6c,0x8e,0x5c,0xcc,0x22,0x6c,0x01,0x40,0x73,0xad,0xc1,0xce,0x47,
        0x0b,0xdf,0x9c,0xa5
    };
    const uint8_t weakPlaintext[] = {'l','e','g','a','c','y',' ','c','i','p','h','e','r',' ','t','e','s','t',0};
    key = pgmmv::ResourceKey{};
    key.available = true;
    require(pgmmv::decodeResource(weakResource, sizeof(weakResource), key, output)
        == pgmmv::ResourceStatus::Decoded, "keyless legacy resource rejected");
    require(output == std::vector<uint8_t>(weakPlaintext, weakPlaintext + sizeof(weakPlaintext)),
        "keyless legacy plaintext mismatch");
    key.size = 16;
    key.bytes[0] = 'a'; key.bytes[1] = 'b'; key.bytes[2] = 'c';
    require(pgmmv::decodeResource(weakResource, sizeof(weakResource), key, output)
        == pgmmv::ResourceStatus::Decoded, "short padded key rejected");
    require(output == std::vector<uint8_t>(weakPlaintext, weakPlaintext + sizeof(weakPlaintext)),
        "short padded key used the wrong cipher");
    key.bytes.fill(0);
    const uint8_t eightByteKey[] = {'1','2','3','4','5','6','7','8'};
    std::copy(eightByteKey, eightByteKey + 8, key.bytes.begin());
    require(pgmmv::decodeResource(weakResource, sizeof(weakResource), key, output)
        == pgmmv::ResourceStatus::Decoded
        && output == std::vector<uint8_t>(weakPlaintext, weakPlaintext + sizeof(weakPlaintext)),
        "eight-byte boundary did not use the weak cipher");
    key.bytes[15] = 255;
    require(pgmmv::decodeResource(weakResource, sizeof(weakResource), key, output)
        == pgmmv::ResourceStatus::Decoded
        && output == std::vector<uint8_t>(weakPlaintext, weakPlaintext + sizeof(weakPlaintext)),
        "bytes after a key terminator changed the weak cipher");
    key.bytes[15] = 0;
    const uint8_t nineByteResource[] = {
        'e','n','c',1,0xc4,0x6e,0x36,0x7a,0x6d,0xaf,0x05,0xe1,0x8d,0x2c,0xd9,0x4d,
        0x2d,0x8d,0x77,0xc2
    };
    const uint8_t nineBytePlaintext[] = {'n','i','n','e','-','b','y','t','e',' ','k','e','y',0,255};
    key.bytes[8] = '9';
    require(pgmmv::decodeResource(nineByteResource, sizeof(nineByteResource), key, output)
        == pgmmv::ResourceStatus::Decoded
        && output == std::vector<uint8_t>(nineBytePlaintext, nineBytePlaintext + sizeof(nineBytePlaintext)),
        "nine-byte boundary did not use Twofish128");

    // info.json stores the metadata as padded base64.
    const char base64Key[] = "Q7mtobfkjnNFFjyxZyEJSA==";
    require(!pgmmv::loadResourceKey(base64Key, 24, key) && key.available && key.size == 16
        && std::equal(expectedKey.begin(), expectedKey.end(), key.bytes.begin()), "base64 key mismatch");
    const char base64Key32[] = "Trqorqrji3BF6jmhaiYMS/AivVxrdzgTqLu6X2i/9Zw=";
    require(!pgmmv::loadResourceKey(base64Key32, 44, key) && key.size == 32, "32-byte base64 key rejected");
    for (size_t i = 0; i < 32; ++i) require(key.bytes[i] == i + 1, "32-byte base64 key mismatch");
    require(!pgmmv::loadResourceKey(nullptr, 0, key) && key.available && key.size == 0,
        "missing key did not select the legacy cipher");
    require(pgmmv::loadResourceKey("Q7mtobfkjnNFFjyxZyEJSA=", 23, key) && !key.available,
        "unpadded base64 key accepted");
    require(pgmmv::loadResourceKey("Q7mtobfkjnNFFjyxZyEJSAAA", 24, key), "unpadded 18-byte key accepted");
    require(pgmmv::loadResourceKey("Q7mtobfkjnNFFjyxZy=JSA==", 24, key), "padding inside a key accepted");
    require(pgmmv::loadResourceKey("Q7mtobfkjnNFFjyxZy\0JSA==", 24, key), "NUL inside a key accepted");
    require(pgmmv::loadResourceKey("Q7mtobfkjnNFFjyxZy-JSA==", 24, key), "URL-safe base64 accepted");

    std::puts("encrypted resource tests passed");
}
