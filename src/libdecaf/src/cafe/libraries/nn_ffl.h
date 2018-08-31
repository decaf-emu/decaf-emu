#pragma once
#include <array>
#include <cstdint>
#include <common/structsize.h>

namespace cafe::nn
{

// This structure is intentionally little-endian as the data
//  is stored in a cross-platform manner for multiple devices.
struct FFLStoreData
{
   uint32_t miiId;
   std::array<uint8_t, 8> systemId;
   uint32_t importantUnk1;
   std::array<uint8_t, 6> deviceHash;
   UNKNOWN(4);
   std::array<char16_t, 10> name;
   UNKNOWN(10);
   uint32_t importantUnk2;
   UNKNOWN(12);
   std::array<char16_t, 10> creator;
   UNKNOWN(2);
   uint16_t crc;
};
CHECK_OFFSET(FFLStoreData, 0x00, miiId);
CHECK_OFFSET(FFLStoreData, 0x04, systemId);
CHECK_OFFSET(FFLStoreData, 0x0C, importantUnk1);
CHECK_OFFSET(FFLStoreData, 0x10, deviceHash);
CHECK_OFFSET(FFLStoreData, 0x1A, name);
CHECK_OFFSET(FFLStoreData, 0x38, importantUnk2);
CHECK_OFFSET(FFLStoreData, 0x48, creator);
CHECK_OFFSET(FFLStoreData, 0x5E, crc);
CHECK_SIZE(FFLStoreData, 0x60);

} // namespace cafe::nn
