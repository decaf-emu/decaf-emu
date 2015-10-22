#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include "modules/gx2/gx2_draw.h"
#include "dx12_state.h"
#include "dx12_fetchshader.h"
#include "dx12_colorbuffer.h"
#include "dx12_depthbuffer.h"
#include "dx12_utils.h"
#include "utils/byte_swap.h"


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
   // TODO: GX2ClearBuffersEx depth/stencil clearing

   auto hostColorBuffer = dx::getColorBuffer(colorBuffer);

   const float clearColor[] = { red, green, blue, alpha };
   gDX.commandList->ClearRenderTargetView(*hostColorBuffer->rtv, clearColor, 0, nullptr);
}


void
GX2ClearColor(GX2ColorBuffer *colorBuffer,
              float red, float green, float blue, float alpha)
{
   auto hostColorBuffer = dx::getColorBuffer(colorBuffer);

   const float clearColor[] = { red, green, blue, alpha };
   gDX.commandList->ClearRenderTargetView(*hostColorBuffer->rtv, clearColor, 0, nullptr);
}


void
GX2ClearDepthStencilEx(GX2DepthBuffer *depthBuffer,
                       float depth,
                       uint8_t stencil,
                       GX2ClearFlags::Flags unk2)
{
   // TODO: GX2ClearDepthStencilEx depth/stencil clearing
}


void
GX2SetAttribBuffer(uint32_t index,
                   uint32_t size,
                   uint32_t stride,
                   void *buffer)
{
   auto& attribData = gDX.state.attribBuffers[index];
   attribData.size = size;
   attribData.stride = stride;
   attribData.buffer = buffer;
}


void
GX2DrawEx(GX2PrimitiveMode::Mode mode,
          uint32_t numVertices,
          uint32_t offset,
          uint32_t numInstances)
{
   // TODO: GX2DrawEx

   dx::updateRenderTargets();
   dx::updatePipeline();
   dx::updateBuffers();

   gDX.commandList->IASetPrimitiveTopology(
      dx12MakePrimitiveTopology(mode));

   gDX.commandList->DrawInstanced(numVertices, numInstances, offset, 0);

}

void
GX2DrawIndexedEx(GX2PrimitiveMode::Mode mode,
                 uint32_t numVertices,
                 GX2IndexType::Type indexType,
                 void *indices,
                 uint32_t offset,
                 uint32_t numInstances)
{
   // TODO: GX2DrawIndexedEx

   dx::updateRenderTargets();
   dx::updatePipeline();
   dx::updateBuffers();

   gDX.commandList->IASetPrimitiveTopology(
      dx12MakePrimitiveTopology(mode));

   switch (indexType) {
   case GX2IndexType::U16:
   {
      auto indexAlloc = gDX.ppcVertexBuffer->get(DXGI_FORMAT_R16_UINT, numVertices * sizeof(uint16_t), nullptr);
      auto indexBuffer = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(indexAlloc));
      auto inBuffer = static_cast<uint16_t*>(indices);
      for (auto i = 0u; i < numVertices; ++i) {
         *indexBuffer++ = byte_swap(*inBuffer++);
      }
      gDX.commandList->IASetIndexBuffer(indexAlloc);
      break;
   }
   default:
      throw;
   }

   gDX.commandList->DrawIndexedInstanced(numVertices, numInstances, 0, offset, 0);
}

#endif
