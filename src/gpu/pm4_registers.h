#pragma once
#include "types.h"

namespace pm4
{

namespace Register
{
enum Value : uint32_t
{
   // Config Registers
   ConfigRegisterBase = 0x8000,
   PrimitiveType = 0x8958,
   NumIndices = 0x8970,
   ConfigRegisterEnd = 0x8a000,

   // Context Registers
   ContextRegisterBase = 0x28000,
   PrimitiveRestartIndex = 0x2840c,
   Blend0Control = 0x028780,
   Blend1Control = 0x028784,
   Blend2Control = 0x028788,
   Blend3Control = 0x02878C,
   Blend4Control = 0x028790,
   Blend5Control = 0x028794,
   Blend6Control = 0x028798,
   Blend7Control = 0x02879C,
   ContextRegisterEnd = 0x29000,
};
}

// VGT_DMA_INDEX_TYPE
union DmaIndexType
{
   uint32_t value;

   struct
   {
      uint32_t indexType : 2;
      uint32_t swapMode : 2;
      uint32_t : 28;
   };
};

// VGT_DRAW_INITIATOR
union DrawInitiator
{
   uint32_t value;

   struct
   {
      uint32_t sourceSelect : 2;
      uint32_t majorMode : 2;
      uint32_t spriteEnabled : 1;
      uint32_t notEndOfPacket : 1;
      uint32_t useOpaque : 1;
      uint32_t : 25;
   };
};

} // namespace pm4
