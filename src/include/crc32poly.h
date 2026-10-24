#pragma once

// The reflected CRC-32 polynomial (IEEE 802.3) the engine's string hashes build their tables from:
// the GFF and key-table label hashes and the global variable tables' name hash.
enum : unsigned int {
    CRC32_POLYNOMIAL = 0xedb88320u,
};
