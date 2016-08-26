#pragma once
#include "common/structsize.h"

// This structure is intentionally little-endian as the data
//  is stored in a cross-platform manner for multiple devices.
struct FFLStoreData
{
   uint8_t miiId[4];
   uint8_t systemId[8];
   uint32_t unk1;
   uint8_t deviceHash[6];
   uint16_t unk2;
   uint16_t unk3;
   char16_t name[10];
   uint16_t unk4;
   uint32_t unk5;
   uint32_t unk6;
   uint32_t unk7;
   uint32_t unk8;
   uint32_t unk9;
   uint32_t unk10;
   char16_t creator[10];
   uint32_t unk11;
};
CHECK_SIZE(FFLStoreData, 96);
