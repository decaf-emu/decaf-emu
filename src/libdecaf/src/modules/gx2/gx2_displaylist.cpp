#include "gx2_displaylist.h"
#include "gx2_enum_string.h"
#include "gx2_internal_cbpool.h"
#include "gx2_shaders.h"
#include "gx2_state.h"
#include "modules/coreinit/coreinit_core.h"

#include <common/align.h>
#include <common/decaf_assert.h>
#include <array>

namespace gx2
{

void
GX2BeginDisplayList(void *displayList,
                    uint32_t bytes)
{
   GX2BeginDisplayListEx(displayList, bytes, TRUE);
}

void
GX2BeginDisplayListEx(void *displayList,
                      uint32_t bytes,
                      BOOL unk1)
{
   internal::beginUserCommandBuffer(reinterpret_cast<uint32_t *>(displayList), bytes / 4);
}

uint32_t
GX2EndDisplayList(void *displayList)
{
   return internal::endUserCommandBuffer(reinterpret_cast<uint32_t*>(displayList)) * 4;
}

BOOL
GX2GetDisplayListWriteStatus()
{
   return internal::getUserCommandBuffer(nullptr, nullptr) ? TRUE : FALSE;
}

BOOL
GX2GetCurrentDisplayList(be_ptr<void> *outDisplayList,
                         be_val<uint32_t> *outSize)
{
   uint32_t *displayList = nullptr;
   uint32_t size = 0;

   if (!internal::getUserCommandBuffer(&displayList, &size)) {
      return FALSE;
   }

   if (outDisplayList) {
      *outDisplayList = displayList;
   }

   if (outSize) {
      *outSize = size;
   }

   return TRUE;
}

void
GX2DirectCallDisplayList(void *displayList,
                         uint32_t bytes)
{
   GX2Flush();
   internal::queueDisplayList(reinterpret_cast<uint32_t*>(displayList), bytes / 4);
}

void
GX2CallDisplayList(void *displayList,
                   uint32_t bytes)
{
   internal::writePM4(latte::pm4::IndirectBufferCall {
      displayList,
      bytes / 4
   });
}

void
GX2CopyDisplayList(void *displayList,
                   uint32_t bytes)
{
   // Copy the display list to the current command buffer
   auto words = bytes / 4;
   auto dst = gx2::internal::getCommandBuffer(words);
   memcpy(&dst->buffer[dst->curSize], displayList, bytes);
   dst->curSize += words;
}

void
GX2PatchDisplayList(void *displayList,
                    GX2PatchShaderType type,
                    uint32_t byteOffset,
                    void *shader)
{
   auto addr = 0u;

   switch (type) {
   case GX2PatchShaderType::FetchShader:
   {
      auto fetchShader = reinterpret_cast<GX2FetchShader *>(shader);
      addr = fetchShader->data.getAddress();
      break;
   }
   case GX2PatchShaderType::VertexShader:
   {
      auto vertexShader = reinterpret_cast<GX2VertexShader *>(shader);

      if (vertexShader->gx2rData.buffer) {
         addr = vertexShader->gx2rData.buffer.getAddress();
      } else {
         addr = vertexShader->data.getAddress();
      }

      break;
   }
   case GX2PatchShaderType::GeometryVertexShader:
   {
      auto geometryShader = reinterpret_cast<GX2GeometryShader *>(shader);

      if (geometryShader->gx2rVertexShaderData.buffer) {
         addr = geometryShader->gx2rVertexShaderData.buffer.getAddress();
      } else {
         addr = geometryShader->vertexShaderData.getAddress();
      }

      break;
   }
   case GX2PatchShaderType::GeometryShader:
   {
      auto geometryShader = reinterpret_cast<GX2GeometryShader *>(shader);

      if (geometryShader->gx2rData.buffer) {
         addr = geometryShader->gx2rData.buffer.getAddress();
      } else {
         addr = geometryShader->data.getAddress();
      }

      break;
   }
   case GX2PatchShaderType::PixelShader:
   {
      auto pixelShader = reinterpret_cast<GX2PixelShader *>(shader);

      if (pixelShader->gx2rData.buffer) {
         addr = pixelShader->gx2rData.buffer.getAddress();
      } else {
         addr = pixelShader->data.getAddress();
      }

      break;
   }
   default:
      decaf_abort(fmt::format("Unsupported GX2PatchShaderType {}", enumAsString(type)));
   }

   // Apply the actual patch
   auto words = reinterpret_cast<be_val<uint32_t> *>(displayList);
   auto idx = byteOffset / 4;
   words[idx + 2] = addr >> 8;
}

} // namespace gx2
