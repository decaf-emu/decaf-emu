#include "../gx2.h"
#ifdef GX2_DX12

#include "../gx2_draw.h"
#include "dx12_state.h"
#include "dx12_fetchshader.h"
#include "dx12_colorbuffer.h"
#include "dx12_depthbuffer.h"
#include "gpu/latte.h"

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
GX2SetAttribBuffer(
   uint32_t index,
   uint32_t size,
   uint32_t stride,
   void *buffer)
{
   auto& attribData = gDX.state.attribBuffers[index];
   attribData.index = index;
   attribData.size = size;
   attribData.stride = stride;
   attribData.buffer = buffer;
}

void
GX2DrawEx(
   GX2PrimitiveMode::Mode mode,
   uint32_t numVertices,
   uint32_t offset,
   uint32_t numInstances)
{
   // TODO: GX2DrawEx
   dx::updateRenderTargets();
   dx::updateBuffers();

   switch (mode) {
   case GX2PrimitiveMode::Triangles:
      gDX.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      break;
   case GX2PrimitiveMode::TriangleStrip:
      gDX.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      break;
   default:
      assert(0);
   }
   
   gDX.commandList->DrawInstanced(numVertices, numInstances, offset, 0);

   // Print Fetch Shader Data
   {
      auto shader = gDX.state.fetchShader;
      auto dataPtr = (FetchShaderInfo*)(void*)shader->data;
      printf("Fetch Shader:\n");
      printf("  Type: %d\n", dataPtr->type);
      printf("  Tess Mode: %d\n", dataPtr->tessMode);
      for (auto i = 0u; i < shader->attribCount; ++i) {
         auto attrib = dataPtr->attribs[i];
         printf("  Attrib[%d] L:%d, B:%d, O:%d, F:%d, T:%d, A:%d, M:%08x, E:%d\n", i,
            (int32_t)attrib.location,
            (int32_t)attrib.buffer,
            (int32_t)attrib.offset,
            (int32_t)attrib.format,
            (int32_t)attrib.type,
            (int32_t)attrib.aluDivisor,
            (uint32_t)attrib.mask,
            (int32_t)attrib.endianSwap);
      }
   }
   // Print Vertex Shader Data
   {
      auto shader = gDX.state.vertexShader;
      std::string code;
      latte::disassemble(code, { (uint8_t*)(void*)shader->data, shader->size });
      std::cout << "Vertex Shader:" << std::endl << code << std::endl;
   }
   // Print Pixel Shader Data
   {
      auto shader = gDX.state.pixelShader;
      std::string code;
      latte::disassemble(code, { (uint8_t*)(void*)shader->data, shader->size });
      std::cout << "Pixel Shader:" << std::endl << code << std::endl;
   }
}

#endif
