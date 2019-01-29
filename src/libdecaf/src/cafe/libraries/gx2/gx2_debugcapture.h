#pragma once
#include "gx2_enum.h"
#include "gx2r_resource.h"

#include "cafe/cafe_ppc_interface_varargs.h"
#include "cafe/libraries/tcl/tcl_ring.h"

#include <libcpu/be2_struct.h>
#include <string_view>

namespace cafe::gx2
{

struct GX2Surface;

using GX2DebugCaptureInterfaceOnShutdownFn =
   virt_func_ptr<void ()>;

using GX2DebugCaptureInterfaceSetAllocatorFn =
   virt_func_ptr<void (GX2RAllocFuncPtr allocFn,
                       GX2RFreeFuncPtr freeFn)>;

using GX2DebugCaptureInterfaceOnCaptureStartFn =
   virt_func_ptr<void (virt_ptr<const char> filename)>;

using GX2DebugCaptureInterfaceOnCaptureEndFn =
   virt_func_ptr<void ()>;

using GX2DebugCaptureInterfaceIsCaptureEnabledFn =
   virt_func_ptr<BOOL ()>;

using GX2DebugCaptureInterfaceOnAllocFn =
   virt_func_ptr<void (virt_ptr<void> ptr, uint32_t size, uint32_t align)>;

using GX2DebugCaptureInterfaceOnFreeFn =
   virt_func_ptr<void (virt_ptr<void> ptr)>;

using GX2DebugCaptureInterfaceOnInvalidateFn =
   virt_func_ptr<void (virt_ptr<void> ptr, uint32_t size)>;

// Note: Real API from gx2.rpl does not actually pass drc scanbuffer, but it
// would be useful for our pm4 capture to have it.
using GX2DebugCaptureInterfaceSetScanbufferFn =
   virt_func_ptr<void (virt_ptr<GX2Surface> tvScanbuffer,
                       virt_ptr<GX2Surface> drcScanbuffer)>;

using GX2DebugCaptureInterfaceOnSwapFn =
   virt_func_ptr<void (virt_ptr<GX2Surface> tvScanbuffer,
                       virt_ptr<GX2Surface> drcScanbuffer)>;

using GX2DebugCaptureInterfaceSubmitFn =
   virt_func_ptr<tcl::TCLStatus (virt_ptr<uint32_t> buffer,
                                 uint32_t numWords,
                                 virt_ptr<tcl::TCLSubmitFlags> submitFlags,
                                 virt_ptr<tcl::TCLTimestamp> lastSubmittedTimestamp)>;

struct GX2DebugCaptureInterface
{
   //! Must be set to GX2DebugCaptureInterfaceVersion::Version1
   be2_val<GX2DebugCaptureInterfaceVersion> version;

   //! Called from GX2Shutdown.
   be2_val<GX2DebugCaptureInterfaceOnShutdownFn> onShutdown;

   //! Called from GX2DebugSetCaptureInterface with the default gx2 allocators.
   be2_val<GX2DebugCaptureInterfaceSetAllocatorFn> setAllocator;

   //! Called from GX2DebugCaptureStart, the filename is first argument passed
   //! in to GX2DebugCaptureStart.
   be2_val<GX2DebugCaptureInterfaceOnCaptureStartFn> onCaptureStart;

   //! Called from GX2DebugCaptureEnd.
   be2_val<GX2DebugCaptureInterfaceOnCaptureEndFn> onCaptureEnd;

   //! Check if capture is enabled.
   be2_val<GX2DebugCaptureInterfaceIsCaptureEnabledFn> isCaptureEnabled;

   //! Called when graphics memory is allocated.
   be2_val<GX2DebugCaptureInterfaceOnAllocFn> onAlloc;

   //! Called when graphics memory is freed.
   be2_val<GX2DebugCaptureInterfaceOnFreeFn> onFree;

   //! Called when graphics memory is invalidated.
   be2_val<GX2DebugCaptureInterfaceOnInvalidateFn> onInvalidate;

   //! Called from GX2DebugCaptureStart with the TV scan buffer.
   be2_val<GX2DebugCaptureInterfaceSetScanbufferFn> setScanbuffer;

   //! Called from GX2SwapScanBuffers with the TV scan buffer.
   be2_val<GX2DebugCaptureInterfaceOnSwapFn> onSwapScanBuffers;

   //! Called when a command buffer is ready to be submitted to ring buffer.
   //! Note that it seems we must call TCLSubmitToRing from this callback
   //! because gx2 will not do it when capturing.
   be2_val<GX2DebugCaptureInterfaceSubmitFn> submitToRing;
};
CHECK_OFFSET(GX2DebugCaptureInterface, 0x00, version);
CHECK_OFFSET(GX2DebugCaptureInterface, 0x04, onShutdown);
CHECK_OFFSET(GX2DebugCaptureInterface, 0x08, setAllocator);
CHECK_OFFSET(GX2DebugCaptureInterface, 0x0C, onCaptureStart);
CHECK_OFFSET(GX2DebugCaptureInterface, 0x10, onCaptureEnd);
CHECK_OFFSET(GX2DebugCaptureInterface, 0x14, isCaptureEnabled);
CHECK_OFFSET(GX2DebugCaptureInterface, 0x18, onAlloc);
CHECK_OFFSET(GX2DebugCaptureInterface, 0x1C, onFree);
CHECK_OFFSET(GX2DebugCaptureInterface, 0x20, onInvalidate);
CHECK_OFFSET(GX2DebugCaptureInterface, 0x24, setScanbuffer);
CHECK_OFFSET(GX2DebugCaptureInterface, 0x28, onSwapScanBuffers);
CHECK_OFFSET(GX2DebugCaptureInterface, 0x2C, submitToRing);
CHECK_SIZE(GX2DebugCaptureInterface, 0x30);

BOOL
GX2DebugSetCaptureInterface(virt_ptr<GX2DebugCaptureInterface> captureInterface);

void
GX2DebugCaptureStart(virt_ptr<const char> filename,
                     BOOL noCallDrawDone);

void
GX2DebugCaptureEnd(BOOL noCallDrawDone);

void
GX2DebugCaptureFrame(virt_ptr<const char> filename);

void
GX2DebugCaptureFrames(virt_ptr<const char> filename,
                      uint32_t numFrames);

void
GX2DebugTagUserString(GX2DebugUserTag tag,
                      virt_ptr<const char> fmt,
                      var_args va);

void
GX2DebugTagUserStringVA(GX2DebugUserTag tag,
                        virt_ptr<const char> fmt,
                        virt_ptr<va_list> vaList);

void
GX2NotifyMemAlloc(virt_ptr<void> ptr,
                  uint32_t size,
                  uint32_t align);

void
GX2NotifyMemFree(virt_ptr<void> ptr);

namespace internal
{

BOOL
debugCaptureEnabled();

void
debugCaptureAlloc(virt_ptr<void> ptr,
                  uint32_t size,
                  uint32_t align);

void
debugCaptureFree(virt_ptr<void> ptr);

void
debugCaptureInvalidate(virt_ptr<void> ptr,
                       uint32_t size);

void
debugCaptureShutdown();

tcl::TCLStatus
debugCaptureSubmit(virt_ptr<uint32_t> buffer,
                   uint32_t numWords,
                   virt_ptr<tcl::TCLSubmitFlags> submitFlags,
                   virt_ptr<tcl::TCLTimestamp> lastSubmittedTimestamp);

void
debugCaptureSwap(virt_ptr<GX2Surface> tvScanbuffer,
                 virt_ptr<GX2Surface> drcScanbuffer);

void
debugCaptureTagGroup(GX2DebugTag tagId,
                     std::string_view str = {});

template<typename... Args>
inline void
debugCaptureTagGroup(GX2DebugTag tagId,
                     const char *fmt,
                     Args &&... args)
{
   if (debugCaptureEnabled()) {
      debugCaptureTagGroup(tagId,
                           std::string_view { fmt::format(fmt, args...) });
   }
}

} // namespace internal

} // namespace cafe::gx2
