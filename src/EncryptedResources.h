// Resource decoding for the Linux runtime. See docs/format.md for provenance.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pgmmv {

struct ResourceKey {
    std::array<uint8_t, 32> bytes{};
    size_t size = 0;
    bool available = false;
};

enum class ResourceStatus { Plain, Decoded, InvalidHeader, MissingKey };

// Input is the binary result of decoding info.json's base64 key, not a password.
// Supports 16- and 32-byte metadata. An available key of size zero selects
// the legacy weak cipher used by keyless builds.
bool decodeResourceKey(const uint8_t *encoded, size_t size, ResourceKey &key);

// Reads the base64 "key" value of data/info.json. A null value, for builds
// without key metadata, selects the legacy weak cipher. Returns nullptr on
// success, otherwise a message for the user.
const char *loadResourceKey(const char *base64, size_t length, ResourceKey &key);

// Plain files and failures leave output unchanged. Each call owns its cipher
// state, so parallel texture/audio loading does not mutate the game key.
ResourceStatus decodeResource(const uint8_t *data, size_t size,
    const ResourceKey &key, std::vector<uint8_t> &output);

}
