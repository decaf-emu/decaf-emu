#pragma once
#include "latte_enum_pm4.h"

#include <common/bitfield.h>
#include <cstdint>

namespace latte
{

namespace pm4
{

#pragma pack(push, 1)

BITFIELD_BEG(Header, uint32_t)
   BITFIELD_ENTRY(30, 2, PacketType, type);
BITFIELD_END

BITFIELD_BEG(HeaderType0, uint32_t)
   BITFIELD_ENTRY(0, 16, uint32_t, baseIndex);
   BITFIELD_ENTRY(16, 14, uint32_t, count);
   BITFIELD_ENTRY(30, 2, PacketType, type);
BITFIELD_END

BITFIELD_BEG(HeaderType2, uint32_t)
   BITFIELD_ENTRY(30, 2, PacketType, type);
BITFIELD_END

BITFIELD_BEG(HeaderType3, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, predicate);
   BITFIELD_ENTRY(8, 8, IT_OPCODE, opcode);
   BITFIELD_ENTRY(16, 14, uint32_t, size);
   BITFIELD_ENTRY(30, 2, PacketType, type);
BITFIELD_END

#pragma pack(pop)

} // namespace pm4

} // namespace latte
