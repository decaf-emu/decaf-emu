#pragma once
#include "systemtypes.h"

enum class SharedType
{
   Unk0,
   Unk1,
   Unk2,
   Unk3
};

BOOL
OSGetSharedData(SharedType type, uint32_t, be_val<uint32_t> *addr, be_val<uint32_t> *size);
