#include "gx2.h"
#include "gx2_displaylist.h"
#include "gx2_enum_string.h"
#include "gx2_fetchshader.h"
#include "gx2_internal_cbpool.h"
#include "gx2_shaders.h"
#include "gx2_state.h"

#include <array>
#include <common/align.h>
#include <common/decaf_assert.h>
#include <fmt/format.h>

namespace cafe::gx2
{

void
GX2BeginDisplayList(virt_ptr<void> displayList,
                    uint32_t bytes)
{
   GX2BeginDisplayListEx(displayList, bytes, TRUE);
}

void
GX2BeginDisplayListEx(virt_ptr<void> displayList,
                      uint32_t bytes,
                      BOOL unk1)
{
   internal::beginUserCommandBuffer(virt_cast<uint32_t *>(displayList).getRawPointer(),
                                    bytes / 4);
}

uint32_t
GX2EndDisplayList(virt_ptr<void> displayList)
{
   return internal::endUserCommandBuffer(virt_cast<uint32_t *>(displayList).getRawPointer()) * 4;
}

BOOL
GX2GetDisplayListWriteStatus()
{
   return internal::getUserCommandBuffer(nullptr, nullptr) ? TRUE : FALSE;
}

BOOL
GX2GetCurrentDisplayList(virt_ptr<virt_ptr<void>> outDisplayList,
                         virt_ptr<uint32_t> outSize)
{
   uint32_t *displayList = nullptr;
   uint32_t size = 0;

   if (!internal::getUserCommandBuffer(&displayList, &size)) {
      return FALSE;
   }

   if (outDisplayList) {
      *outDisplayList = virt_cast<void *>(cpu::translate(displayList));
   }

   if (outSize) {
      *outSize = size;
   }

   return TRUE;
}

void
GX2DirectCallDisplayList(virt_ptr<void> displayList,
                         uint32_t bytes)
{
   GX2Flush();
   internal::queueDisplayList(virt_cast<uint32_t *>(displayList).getRawPointer(),
                              bytes / 4);
}

void
GX2CallDisplayList(virt_ptr<void> displayList,
                   uint32_t bytes)
{
   internal::writePM4(latte::pm4::IndirectBufferCall {
      displayList.getRawPointer(),
      bytes / 4
   });
}

void
GX2CopyDisplayList(virt_ptr<void> displayList,
                   uint32_t bytes)
{
   // Copy the display list to the current command buffer
   auto words = bytes / 4;
   auto dst = gx2::internal::getCommandBuffer(words);
   memcpy(&dst->buffer[dst->curSize], displayList.getRawPointer(), bytes);
   dst->curSize += words;
}

void
GX2PatchDisplayList(virt_ptr<void> displayList,
                    GX2PatchShaderType type,
                    uint32_t byteOffset,
                    virt_ptr<void> shader)
{
   auto addr = virt_addr { 0u };

   switch (type) {
   case GX2PatchShaderType::FetchShader:
   {
      auto fetchShader = virt_cast<GX2FetchShader *>(shader);
      addr = virt_cast<virt_addr>(fetchShader->data);
      break;
   }
   case GX2PatchShaderType::VertexShader:
   {
      auto vertexShader = virt_cast<GX2VertexShader *>(shader);

      if (vertexShader->gx2rData.buffer) {
         addr = virt_cast<virt_addr>(vertexShader->gx2rData.buffer);
      } else {
         addr = virt_cast<virt_addr>(vertexShader->data);
      }

      break;
   }
   case GX2PatchShaderType::GeometryVertexShader:
   {
      auto geometryShader = virt_cast<GX2GeometryShader *>(shader);

      if (geometryShader->gx2rVertexShaderData.buffer) {
         addr = virt_cast<virt_addr>(geometryShader->gx2rVertexShaderData.buffer);
      } else {
         addr = virt_cast<virt_addr>(geometryShader->vertexShaderData);
      }

      break;
   }
   case GX2PatchShaderType::GeometryShader:
   {
      auto geometryShader = virt_cast<GX2GeometryShader *>(shader);

      if (geometryShader->gx2rData.buffer) {
         addr = virt_cast<virt_addr>(geometryShader->gx2rData.buffer);
      } else {
         addr = virt_cast<virt_addr>(geometryShader->data);
      }

      break;
   }
   case GX2PatchShaderType::PixelShader:
   {
      auto pixelShader = virt_cast<GX2PixelShader *>(shader);

      if (pixelShader->gx2rData.buffer) {
         addr = virt_cast<virt_addr>(pixelShader->gx2rData.buffer);
      } else {
         addr = virt_cast<virt_addr>(pixelShader->data);
      }

      break;
   }
   default:
      decaf_abort(fmt::format("Unsupported GX2PatchShaderType {}", to_string(type)));
   }

   // Apply the actual patch
   auto words = virt_cast<be_val<uint32_t> *>(displayList);
   auto idx = byteOffset / 4;
   words[idx + 2] = addr >> 8;
}

void
Library::registerDisplayListSymbols()
{
   RegisterFunctionExport(GX2BeginDisplayListEx);
   RegisterFunctionExport(GX2BeginDisplayList);
   RegisterFunctionExport(GX2EndDisplayList);
   RegisterFunctionExport(GX2DirectCallDisplayList);
   RegisterFunctionExport(GX2CallDisplayList);
   RegisterFunctionExport(GX2GetDisplayListWriteStatus);
   RegisterFunctionExport(GX2GetCurrentDisplayList);
   RegisterFunctionExport(GX2CopyDisplayList);
   RegisterFunctionExport(GX2PatchDisplayList);
}

} // namespace cafe::gx2
