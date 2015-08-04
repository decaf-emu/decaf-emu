#include "gx2.h"
#include "gx2_draw.h"
#include "gx2internal.h"

void
GX2SetClearDepthStencil(GX2DepthBuffer *depthBuffer,
                        float depth,
                        uint8_t stencil)
{
   // TODO: GX2SetClearDepthStencil
}

void
GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
                  GX2DepthBuffer *depthBuffer,
                  float red, float green, float blue, float alpha,
                  float depth,
                  uint8_t unk1,
                  GX2ClearFlags::Flags flags)
{
   // TODO: GX2ClearBuffersEx

   /*
   gGX2State.contextState = nullptr;

   glClearColor(red, green, blue, alpha);
   glClearDepth(depth);

   GLbitfield clearMask = 0;

   if (colorBuffer) {
      GLuint hostColorBuffer = getColorBuffer(colorBuffer);
      
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHEMENT0, GL_TEXTURE_2D, hostColorBuffer, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHEMENT1, 0, 0, 0);

      clearMask |= GL_COLOR_BUFFER_BIT;
   }

   if (depthBuffer) {
      if (flags & GX2ClearFlags::Depth) {
         // Depth

         clearMask |= GL_DEPTH_BUFFER_BIT;
      }
   }

   glClear(clearMask);
   */
}

void
GX2SetAttribBuffer(uint32_t unk1,
                   uint32_t unk2,
                   uint32_t unk3,
                   void *buffer)
{
   // TODO: GX2SetAttribBuffer
}

void
GX2DrawEx(GX2PrimitiveMode::Mode mode,
          uint32_t unk1,
          uint32_t unk2,
          uint32_t unk3)
{
   // TODO: GX2DrawEx
}

void GX2::registerDrawFunctions()
{
   RegisterKernelFunction(GX2ClearBuffersEx);
   RegisterKernelFunction(GX2SetClearDepthStencil);
   RegisterKernelFunction(GX2SetAttribBuffer);
   RegisterKernelFunction(GX2DrawEx);
}
