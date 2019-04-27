#include "gx2.h"
#include "gx2_cbpool.h"
#include "gx2_debugcapture.h"
#include "gx2_display.h"
#include "gx2_event.h"
#include "gx2_state.h"
#include "gx2r_resource.h"

#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/cafe_ppc_interface_varargs.h"
#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_snprintf.h"

#include <common/strutils.h>

namespace cafe::gx2
{

struct StaticDebugCaptureData
{
   be2_val<BOOL> enabled;
   be2_val<uint32_t> previousSwapInterval;
   be2_struct<GX2DebugCaptureInterface> captureInterface;

   be2_val<uint32_t> numCaptureFramesRemaining;
   be2_array<char, 256> pendingCaptureFilename;
};

static virt_ptr<StaticDebugCaptureData>
sDebugCaptureData = nullptr;


/**
 * Initialise the debug capture interface.
 */
BOOL
GX2DebugSetCaptureInterface(virt_ptr<GX2DebugCaptureInterface> captureInterface)
{
   // Normally this is only allowed when:
   // OSGetSecurityLevel () != 1 &&
   // OSGetConsoleType() != 0x13000048 &&
   // OSGetConsoleType() != 0x13000040

   if (!captureInterface ||
       captureInterface->version != GX2DebugCaptureInterfaceVersion::Version1 ||
       !captureInterface->onShutdown ||
       !captureInterface->setAllocator ||
       !captureInterface->onCaptureStart ||
       !captureInterface->onCaptureEnd ||
       !captureInterface->isCaptureEnabled ||
       !captureInterface->onAlloc ||
       !captureInterface->onInvalidate ||
       !captureInterface->setScanbuffer ||
       !captureInterface->onSwapScanBuffers ||
       !captureInterface->submitToRing) {
      return FALSE;
   }

   sDebugCaptureData->captureInterface = *captureInterface;
   sDebugCaptureData->enabled = TRUE;
   cafe::invoke(cpu::this_core::state(),
                sDebugCaptureData->captureInterface.setAllocator,
                internal::getDefaultGx2rAlloc(),
                internal::getDefaultGx2rFree());
   return TRUE;
}


/**
 * Start a debug capture.
 */
void
GX2DebugCaptureStart(virt_ptr<const char> filename,
                     BOOL noCallDrawDone)
{
   if (!sDebugCaptureData->enabled) {
      return;
   }

   if (!noCallDrawDone) {
      GX2DrawDone();

      sDebugCaptureData->previousSwapInterval = GX2GetSwapInterval();
      GX2SetSwapInterval(0);
   }

   cafe::invoke(cpu::this_core::state(),
                sDebugCaptureData->captureInterface.onCaptureStart,
                filename);

   internal::debugCaptureCbPoolPointers();

   cafe::invoke(cpu::this_core::state(),
                sDebugCaptureData->captureInterface.setScanbuffer,
                internal::getTvScanBuffer(),
                internal::getDrcScanBuffer());
}


/**
 * End a debug capture.
 */
void
GX2DebugCaptureEnd(BOOL noCallFlush)
{
   if (!sDebugCaptureData->enabled) {
      return;
   }

   if (!noCallFlush) {
      GX2Flush();
   }

   internal::debugCaptureCbPoolPointersFree();

   cafe::invoke(cpu::this_core::state(),
                sDebugCaptureData->captureInterface.onCaptureEnd);

   GX2SetSwapInterval(sDebugCaptureData->previousSwapInterval);
}


/**
 * Capture the next frame.
 */
void
GX2DebugCaptureFrame(virt_ptr<const char> filename)
{
   if (!sDebugCaptureData->enabled) {
      return;
   }

   GX2DebugCaptureFrames(filename, 1);
}


/**
 * Capture the next n frames.
 */
void
GX2DebugCaptureFrames(virt_ptr<const char> filename,
                      uint32_t numFrames)
{
   if (!sDebugCaptureData->enabled) {
      return;
   }

   string_copy(virt_addrof(sDebugCaptureData->pendingCaptureFilename).get(),
               filename.get(),
               sDebugCaptureData->pendingCaptureFilename.size() - 1);
   sDebugCaptureData->numCaptureFramesRemaining = numFrames;
}


/**
 * Insert a debug tag into the pm4 stream.
 *
 * Only written when debug capture is enabled.
 */
void
GX2DebugTagUserString(GX2DebugUserTag tag,
                      virt_ptr<const char> fmt,
                      var_args va)
{
   if (internal::debugCaptureEnabled()) {
      auto list = make_va_list(va);
      GX2DebugTagUserStringVA(tag, fmt, list);
      free_va_list(list);
   }
}


/**
 * Insert a debug tag into the pm4 stream.
 *
 * Only written when debug capture is enabled.
 */
void
GX2DebugTagUserStringVA(GX2DebugUserTag tag,
                        virt_ptr<const char> fmt,
                        virt_ptr<va_list> vaList)
{
   if (internal::debugCaptureEnabled()) {
      auto buffer = StackArray<char, 0x404> { };
      std::memset(buffer.get(), 0, 0x404);

      if (fmt) {
         coreinit::internal::formatStringV(buffer, 0x3FF, fmt, vaList);
      }

      // Convert string to words!
      auto length = static_cast<uint32_t>(strlen(buffer.get()));
      auto numWords = align_up(length + 1, 4) / 4;
      auto bufferWords = virt_cast<uint32_t *>(buffer);

      // Write NOP packet
      internal::writePM4(latte::pm4::NopBE {
         GX2DebugTag::User | tag,
         gsl::make_span(virt_cast<uint32_t *>(buffer).get(), numWords)
      });
   }
}


/**
 * Notify gx2 of a graphics memory allocation.
 */
void
GX2NotifyMemAlloc(virt_ptr<void> ptr,
                  uint32_t size,
                  uint32_t align)
{
   if (internal::debugCaptureEnabled()) {
      internal::debugCaptureAlloc(ptr, size, align);
   }
}


/**
 * Notify gx2 of a graphics memory free.
 */
void
GX2NotifyMemFree(virt_ptr<void> ptr)
{
   if (internal::debugCaptureEnabled()) {
      internal::debugCaptureFree(ptr);
   }
}


namespace internal
{

BOOL
debugCaptureEnabled()
{
   if (!sDebugCaptureData->enabled) {
      return FALSE;
   }

   return cafe::invoke(cpu::this_core::state(),
                       sDebugCaptureData->captureInterface.isCaptureEnabled);
}

void
debugCaptureAlloc(virt_ptr<void> ptr,
                  uint32_t size,
                  uint32_t align)
{
   cafe::invoke(cpu::this_core::state(),
                sDebugCaptureData->captureInterface.onAlloc,
                ptr, size, align);
}

void
debugCaptureFree(virt_ptr<void> ptr)
{
   cafe::invoke(cpu::this_core::state(),
                sDebugCaptureData->captureInterface.onFree,
                ptr);
}

void
debugCaptureInvalidate(virt_ptr<void> ptr,
                       uint32_t size)
{
   cafe::invoke(cpu::this_core::state(),
                sDebugCaptureData->captureInterface.onInvalidate,
                ptr, size);
}

void
debugCaptureShutdown()
{
   cafe::invoke(cpu::this_core::state(),
                sDebugCaptureData->captureInterface.onShutdown);
   sDebugCaptureData->enabled = FALSE;
}

tcl::TCLStatus
debugCaptureSubmit(virt_ptr<uint32_t> buffer,
                   uint32_t numWords,
                   virt_ptr<tcl::TCLSubmitFlags> submitFlags,
                   virt_ptr<tcl::TCLTimestamp> lastSubmittedTimestamp)
{
   return cafe::invoke(cpu::this_core::state(),
                       sDebugCaptureData->captureInterface.submitToRing,
                       buffer, numWords, submitFlags, lastSubmittedTimestamp);
}

void
debugCaptureSwap(virt_ptr<GX2Surface> tvScanBuffer,
                 virt_ptr<GX2Surface> drcScanBuffer)
{
   if (!sDebugCaptureData->enabled) {
      return;
   }

   auto enabled = debugCaptureEnabled();
   if (!enabled) {
      // Check if we need to start a capture
      if (sDebugCaptureData->numCaptureFramesRemaining) {
         GX2DebugCaptureStart(virt_addrof(sDebugCaptureData->pendingCaptureFilename),
                              FALSE);
      }

      return;
   }

   // Capture frame
   GX2DrawDone();
   cafe::invoke(cpu::this_core::state(),
                sDebugCaptureData->captureInterface.onSwapScanBuffers,
                tvScanBuffer, drcScanBuffer);

   if (!sDebugCaptureData->numCaptureFramesRemaining) {
      return;
   }

   // Check if we need to end a capture
   if (sDebugCaptureData->numCaptureFramesRemaining == 1) {
      GX2DebugCaptureEnd(FALSE);
   }

   sDebugCaptureData->numCaptureFramesRemaining--;
}

void
debugCaptureTagGroup(GX2DebugTag tagId,
                     std::string_view str)
{
   auto id = tagId | GX2DebugTag::Group;

   if (str.empty()) {
      internal::writePM4(latte::pm4::Nop { id, { } });
   } else {
      std::vector<uint32_t> buffer;
      buffer.resize(align_up(str.size() + 1, 4) / 4, 0u);
      std::memcpy(buffer.data(), str.data(), str.size());
      internal::writePM4(latte::pm4::Nop { id, { buffer.data(), buffer.size() } });
   }
}

} // namespace internal

void
Library::registerDebugCaptureSymbols()
{
   RegisterFunctionExportName("_GX2DebugSetCaptureInterface",
                              GX2DebugSetCaptureInterface);
   RegisterFunctionExport(GX2DebugCaptureStart);
   RegisterFunctionExport(GX2DebugCaptureEnd);
   RegisterFunctionExport(GX2DebugCaptureFrame);
   RegisterFunctionExport(GX2DebugCaptureFrames);
   RegisterFunctionExport(GX2DebugTagUserString);
   RegisterFunctionExport(GX2DebugTagUserStringVA);
   RegisterFunctionExport(GX2NotifyMemAlloc);
   RegisterFunctionExport(GX2NotifyMemFree);

   RegisterDataInternal(sDebugCaptureData);
}

} // namespace cafe::gx2
