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
_GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
                  GX2DepthBuffer *depthBuffer,
                  float red, float green, float blue, float alpha,
                  float depth,
                  uint8_t unk1,
                  GX2ClearFlags::Flags flags)
{
   auto hostColorBuffer = dx::getColorBuffer(colorBuffer);

   const float clearColor[] = { red, green, blue, alpha };
   gDX.commandList->ClearRenderTargetView(*hostColorBuffer->rtv, clearColor, 0, nullptr);
}

void
GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
   GX2DepthBuffer *depthBuffer,
   float red, float green, float blue, float alpha,
   float depth,
   uint8_t unk1,
   GX2ClearFlags::Flags flags)
{
   DX_DLCALL(_GX2ClearBuffersEx, colorBuffer, depthBuffer, 
      red, green, blue, alpha, depth, unk1, flags);
}

void
_GX2ClearColor(GX2ColorBuffer *colorBuffer,
              float red, float green, float blue, float alpha)
{
   auto hostColorBuffer = dx::getColorBuffer(colorBuffer);

   const float clearColor[] = { red, green, blue, alpha };
   gDX.commandList->ClearRenderTargetView(*hostColorBuffer->rtv, clearColor, 0, nullptr);
}

void
GX2ClearColor(GX2ColorBuffer *colorBuffer,
   float red, float green, float blue, float alpha)
{
   DX_DLCALL(_GX2ClearColor, colorBuffer, red, green, blue, alpha);
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
_GX2SetAttribBuffer(uint32_t index,
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
GX2SetAttribBuffer(uint32_t index,
   uint32_t size,
   uint32_t stride,
   void *buffer)
{
   DX_DLCALL(_GX2SetAttribBuffer, index, size, stride, buffer);
}

// Decomposes a set of quads to a triangle list
template <typename IndexType>
void
triifiedDraw(GX2PrimitiveMode::Mode mode,
   uint32_t numVertices,
   IndexType *indices,
   uint32_t offset,
   uint32_t numInstances)
{
   gDX.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   uint32_t newNumIndices = 0;
   switch (mode) {
   case GX2PrimitiveMode::Quads:
      newNumIndices = numVertices * 6 / 4;
      break;
   case GX2PrimitiveMode::QuadStrip:
      // Don't actually know how to handle a quad strip...
      //   How the hell do you strip a quad list :S
      throw;
   default:
      // Nobody should be calling me with other modes...
      throw;
   }

   // Always 32-bit indices for now to save scanning the input
   //   indices to see if they will fit into 16-bit indices after expansion.
   auto indexAlloc = gDX.ppcVertexBuffer->get(DXGI_FORMAT_R32_UINT, newNumIndices * sizeof(uint32_t), nullptr);
   auto indicesOut = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(indexAlloc));

   for (auto i = 0u; i < numVertices / 4; ++i) {
      auto index_tl = byte_swap(*indices++);
      auto index_tr = byte_swap(*indices++);
      auto index_bl = byte_swap(*indices++);
      auto index_br = byte_swap(*indices++);
      
      *indicesOut++ = index_tl;
      *indicesOut++ = index_tr;
      *indicesOut++ = index_bl;
      *indicesOut++ = index_bl;
      *indicesOut++ = index_br;
      *indicesOut++ = index_tl;
   }

   gDX.commandList->IASetIndexBuffer(indexAlloc);
   gDX.commandList->DrawIndexedInstanced(newNumIndices, numInstances, 0, offset, 0);
}

void
_GX2DrawEx(GX2PrimitiveMode::Mode mode,
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
GX2DrawEx(GX2PrimitiveMode::Mode mode,
   uint32_t numVertices,
   uint32_t offset,
   uint32_t numInstances)
{
   DX_DLCALL(_GX2DrawEx, mode, numVertices, offset, numInstances);
}

void
_GX2DrawIndexedEx(GX2PrimitiveMode::Mode mode,
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

   if (mode == GX2PrimitiveMode::Quads) {
      switch (indexType) {
      case GX2IndexType::U16:
         return triifiedDraw(mode, numVertices, static_cast<uint16_t*>(indices), offset, numInstances);
      default:
         throw;
      }
   }

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

void
GX2DrawIndexedEx(GX2PrimitiveMode::Mode mode,
   uint32_t numVertices,
   GX2IndexType::Type indexType,
   void *indices,
   uint32_t offset,
   uint32_t numInstances)
{
   DX_DLCALL(_GX2DrawIndexedEx, mode, numVertices, indexType, indices, offset, numInstances);
}

#endif
