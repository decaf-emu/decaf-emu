#include "gx2.h"
#include "gx2_debugcapture.h"
#include "gx2_displaylist.h"
#include "gx2_enum_string.h"
#include "gx2_fetchshader.h"
#include "gx2_cbpool.h"
#include "gx2_shaders.h"
#include "gx2_state.h"

#include "cafe/libraries/coreinit/coreinit_memory.h"

#include <array>
#include <common/align.h>
#include <common/decaf_assert.h>
#include <fmt/format.h>

namespace cafe::gx2
{

using namespace cafe::coreinit;

void
GX2BeginDisplayListEx(virt_ptr<void> displayList,
                      uint32_t bytes,
                      BOOL profilingEnabled)
{
   internal::beginUserCommandBuffer(virt_cast<uint32_t *>(displayList),
                                    bytes,
                                    profilingEnabled);
}

uint32_t
GX2EndDisplayList(virt_ptr<void> displayList)
{
   auto size = internal::endUserCommandBuffer(virt_cast<uint32_t *>(displayList));

   if (internal::debugCaptureEnabled()) {
      internal::debugCaptureInvalidate(displayList, size);
   }

   return size;
}

BOOL
GX2GetDisplayListWriteStatus()
{
   return internal::getActiveCommandBuffer()->isUserBuffer;
}

BOOL
GX2GetCurrentDisplayList(virt_ptr<virt_ptr<void>> outDisplayList,
                         virt_ptr<uint32_t> outSize)
{
   auto cb = internal::getActiveCommandBuffer();

   if (!cb->isUserBuffer) {
      return FALSE;
   }

   if (outDisplayList) {
      *outDisplayList = cb->buffer;
   }

   if (outSize) {
      *outSize = 4 * cb->bufferSizeWords;
   }

   return TRUE;
}

void
GX2DirectCallDisplayList(virt_ptr<void> displayList,
                         uint32_t bytes)
{
   auto cb = internal::getActiveCommandBuffer();
   if (!cb->isUserBuffer) {
      internal::flushCommandBuffer(256, FALSE);
   }

   internal::queueCommandBuffer(virt_cast<uint32_t *>(displayList),
                                bytes / sizeof(uint32_t),
                                nullptr,
                                TRUE);
}

void
GX2CallDisplayList(virt_ptr<void> displayList,
                   uint32_t bytes)
{
   internal::writePM4(latte::pm4::IndirectBufferCall {
      OSEffectiveToPhysical(virt_cast<virt_addr>(displayList)),
      bytes / 4
   });
}

void
GX2CopyDisplayList(virt_ptr<void> displayList,
                   uint32_t bytes)
{
   auto numWords = bytes / 4;
   auto cb = internal::getWriteCommandBuffer(numWords);
   cb->writeGatherPtr.write(virt_cast<uint32_t *>(displayList), numWords);
   cb->cmdSize = numWords;
   decaf_check(cb->cmdSize == cb->cmdSizeTarget);
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
   auto words = virt_cast<uint32_t *>(displayList);
   auto idx = byteOffset / 4;
   words[idx + 2] = OSEffectiveToPhysical(addr) >> 8;
}

void
Library::registerDisplayListSymbols()
{
   RegisterFunctionExport(GX2BeginDisplayListEx);
   RegisterFunctionExport(GX2EndDisplayList);
   RegisterFunctionExport(GX2DirectCallDisplayList);
   RegisterFunctionExport(GX2CallDisplayList);
   RegisterFunctionExport(GX2GetDisplayListWriteStatus);
   RegisterFunctionExport(GX2GetCurrentDisplayList);
   RegisterFunctionExport(GX2CopyDisplayList);
   RegisterFunctionExport(GX2PatchDisplayList);
}

} // namespace cafe::gx2
