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
      Invalid,
      CommandBuffer,
      MemoryLoad,
      RegisterSnapshot,
      SetBuffer,
   };

   Type type;
   uint32_t size;
};

struct CaptureMemoryLoad
{
   enum MemoryType : uint32_t
   {
      Unknown,
      CpuFlush,
      SurfaceSync,
      ShadowState,
      CommandBuffer,
      AttributeBuffer,
      UniformBuffer,
      IndexBuffer,
      Surface,
      FetchShader,
      VertexShader,
      PixelShader,
      GeometryShader,
   };

   MemoryType type;
   uint32_t address;
};

struct CaptureSetBuffer
{
   enum Type : uint32_t
   {
      Invalid,
      TvBuffer,
      DrcBuffer,
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
