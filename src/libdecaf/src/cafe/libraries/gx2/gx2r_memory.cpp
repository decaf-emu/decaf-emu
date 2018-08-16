#include "gx2.h"
#include "gx2_memory.h"
#include "gx2r_memory.h"

namespace cafe::gx2
{

static GX2InvalidateMode
getInvalidateMode(GX2RResourceFlags flags)
{
   auto mode = 0u;

   if (flags & GX2RResourceFlags::BindTexture) {
      mode |= GX2InvalidateMode::Texture;
   }

   if (flags & GX2RResourceFlags::BindColorBuffer) {
      mode |= GX2InvalidateMode::ColorBuffer;
   }

   if (flags & GX2RResourceFlags::BindDepthBuffer) {
      mode |= GX2InvalidateMode::DepthBuffer;
   }

   if (flags & GX2RResourceFlags::BindScanBuffer) {
      mode |= GX2InvalidateMode::ColorBuffer;
      mode |= GX2InvalidateMode::DepthBuffer;
   }

   if (flags & GX2RResourceFlags::BindVertexBuffer) {
      mode |= GX2InvalidateMode::AttributeBuffer;
   }

   if (flags & GX2RResourceFlags::BindIndexBuffer) {
      mode |= GX2InvalidateMode::AttributeBuffer;
   }

   if (flags & GX2RResourceFlags::BindUniformBlock) {
      mode |= GX2InvalidateMode::UniformBlock;
   }

   if (flags & GX2RResourceFlags::BindShaderProgram) {
      mode |= GX2InvalidateMode::Shader;
   }

   if (flags & GX2RResourceFlags::BindStreamOutput) {
      mode |= GX2InvalidateMode::StreamOutBuffer;
   }

   if (flags & GX2RResourceFlags::UsageCpuReadWrite) {
      mode |= GX2InvalidateMode::CPU;
   }

   if (flags & GX2RResourceFlags::DisableCpuInvalidate) {
      // Clear only the CPU bit
      mode &= ~GX2InvalidateMode::CPU;
   }

   if (flags & GX2RResourceFlags::DisableGpuInvalidate) {
      // Clear every bit except CPU
      mode &= GX2InvalidateMode::CPU;
   }

   return static_cast<GX2InvalidateMode>(mode);
}

void
GX2RInvalidateMemory(GX2RResourceFlags flags,
                     virt_ptr<void> buffer,
                     uint32_t size)
{
   GX2Invalidate(getInvalidateMode(flags), buffer, size);
}

void
Library::registerGx2rMemorySymbols()
{
   RegisterFunctionExport(GX2RInvalidateMemory);
}

} // namespace cafe::gx2
