#pragma once
#include <array>
#include <cstdint>

namespace decaf
{

namespace pm4
{

static const std::array<char, 4> CaptureMagic =
{
   'D', 'P', 'M', '4'
};

struct CapturePacket
{
   enum Type : uint32_t
   {
      CommandBuffer = 1,
      MemoryLoad = 2,
   };

   Type type;
   uint32_t size;
};

struct CaptureMemoryLoad
{
   uint32_t address;
};

void
injectCommandBuffer(void *buffer,
                    uint32_t bytes);

} // namespace pm4

} // namespace decaf
