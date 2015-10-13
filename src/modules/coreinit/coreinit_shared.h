#pragma once
#include "systemtypes.h"

namespace OSSharedDataType
{
enum Type
{
   FontChinese    = 0,
   FontKorean     = 1,
   FontStandard   = 2,
   FontTaiwanese  = 3
};
}

BOOL
OSGetSharedData(OSSharedDataType::Type type, uint32_t, be_ptr<uint8_t> *addr, be_val<uint32_t> *size);
