#pragma once
#include "systemtypes.h"

namespace OSSharedDataType
{
enum Type
{
   Unk0,
   Unk1,
   Unk2,
   Unk3
};
}

BOOL
OSGetSharedData(OSSharedDataType::Type type, uint32_t, be_val<uint32_t> *addr, be_val<uint32_t> *size);
