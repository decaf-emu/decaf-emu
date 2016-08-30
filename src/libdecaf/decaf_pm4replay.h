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
      RegisterSnapshot = 3,
      SetBuffer = 4,
   };

   Type type;
   uint32_t size;
};

struct CaptureMemoryLoad
{
   uint32_t address;
};

struct CaptureSetBuffer
{
   enum Type : uint32_t
   {
      TvBuffer = 1,
      DrcBuffer = 2,
   };

   Type type;
   uint32_t address;
   uint32_t size;
   uint32_t renderMode;
   uint32_t surfaceFormat;
   uint32_t bufferingMode;
   uint32_t width;
   uint32_t height;
};

void
injectCommandBuffer(void *buffer,
                    uint32_t bytes);

} // namespace pm4

} // namespace decaf
