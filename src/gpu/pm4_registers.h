#pragma once
#include "types.h"

namespace pm4
{

namespace ConfigRegister
{
enum Value : uint32_t
{
   Base = 0x2000,
   PrimitiveType = 0x2256,
};
}

namespace ContextRegister
{
enum Value : uint32_t
{
   Base = 0xA000,
   PrimitiveRestartIndex = 0xA103,
   Blend0Control = 0xA1E0,
   Blend1Control = 0xA1E1,
   Blend2Control = 0xA1E2,
   Blend3Control = 0xA1E3,
   Blend4Control = 0xA1E4,
   Blend5Control = 0xA1E5,
   Blend6Control = 0xA1E6,
   Blend7Control = 0xA1E7,
};
}

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
