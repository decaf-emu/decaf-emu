#include "gx2.h"
#include "gx2_debugcapture.h"
#include "gx2_displaylist.h"
#include "gx2_event.h"
#include "gx2_cbpool.h"
#include "gx2_internal_pm4cap.h"
#include "gx2_state.h"
#include "gx2_query.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_memory.h"
#include "cafe/libraries/coreinit/coreinit_time.h"
#include "cafe/libraries/tcl/tcl_ring.h"

#include <common/decaf_assert.h>
#include <libcpu/be2_struct.h>
#include <libgpu/latte/latte_pm4.h>
#include <libgpu/latte/latte_pm4_writer.h>

namespace cafe::gx2
{

constexpr auto MinimumCommandBufferNumWords = uint32_t { 0x100 };
constexpr auto MaximumCommandBufferNumWords = uint32_t { 0x20000 };

using namespace cafe::coreinit;
using namespace cafe::tcl;

struct StaticCbPoolData
{
   be2_struct<internal::ActiveCommandBuffer> mainCoreCommandBuffer;
   be2_array<internal::ActiveCommandBuffer, 3> activeCommandBuffer;
   be2_val<BOOL> profilingWasEnabledBeforeUserDisplayList;
   be2_val<virt_addr> gpuLastReadCommandBuffer;
   be2_val<GX2Timestamp> lastSubmittedTimestamp;
};

static virt_ptr<StaticCbPoolData>
sCbPoolData = nullptr;

GX2Timestamp
GX2GetRetiredTimeStamp()
{
   auto timestamp = StackObject<TCLTimestamp> { };
   TCLReadTimestamp(TCLTimestampID::CPRetired, timestamp);
   return *timestamp;
}

GX2Timestamp
GX2GetLastSubmittedTimeStamp()
{
   return sCbPoolData->lastSubmittedTimestamp;
}


/**
 * Submit a user timestamp to the GPU.
 */
void
GX2SubmitUserTimeStamp(virt_ptr<GX2Timestamp> dst,
                       GX2Timestamp timestamp,
                       GX2PipeEvent type,
                       BOOL triggerInterrupt)
{
   using namespace latte;
   using namespace latte::pm4;
   auto addr = OSEffectiveToPhysical(virt_cast<virt_addr>(dst));
   auto dataLo = static_cast<uint32_t>(timestamp & 0xFFFFFFFFu);
   auto dataHi = static_cast<uint32_t>(timestamp >> 32);

   switch (type) {
   case GX2PipeEvent::Top:
   {
      internal::writePM4(MemWrite {
         MW_ADDR_LO::get(0)
            .ADDR_LO(addr >> 2)
            .ENDIAN_SWAP(CB_ENDIAN::SWAP_8IN32),
         MW_ADDR_HI::get(0)
            .CNTR_SEL(MW_WRITE_DATA),
         dataLo, dataHi
      });

      if (triggerInterrupt) {
         internal::writeType0(Register::CP_INT_STATUS,
                              CP_INT_STATUS::get(0)
                              .IB1_INT_STAT(true)
                              .value);
      }
      break;
   }
   case GX2PipeEvent::Bottom:
   case GX2PipeEvent::BottomAfterFlush:
   {
      internal::writePM4(EventWriteEOP {
         VGT_EVENT_INITIATOR::get(0)
            .EVENT_TYPE(type == GX2PipeEvent::BottomAfterFlush ?
                           VGT_EVENT_TYPE::CACHE_FLUSH_AND_INV_TS_EVENT :
                           VGT_EVENT_TYPE::BOTTOM_OF_PIPE_TS)
            .EVENT_INDEX(VGT_EVENT_INDEX::TS),
         EW_ADDR_LO::get(0)
            .ADDR_LO(addr >> 2)
            .ENDIAN_SWAP(CB_ENDIAN::SWAP_8IN32),
         EWP_ADDR_HI::get(0)
            .DATA_SEL(EWP_DATA_64)
            .INT_SEL(triggerInterrupt ?
                        EWP_INT_SEL::EWP_INT_WRITE_CONFIRM :
                        EWP_INT_SEL::EWP_INT_NONE),
            dataLo, dataHi
      });
      break;
   }
   default:
      decaf_abort(fmt::format("Unexpected GX2SubmitUserTimestamp type {}",
                              type));
   }
}


/**
 * Wait for retired timestamp.
 */
BOOL
GX2WaitTimeStamp(GX2Timestamp timestamp)
{
   auto timeoutTicks = coreinit::internal::msToTicks(GX2GetGPUTimeout());
   if (TCLWaitTimestamp(TCLTimestampID::CPRetired, timestamp, timeoutTicks) == TCLStatus::OK) {
      return TRUE;
   }

   // TODO: Set GPU hang state
   return FALSE;
}


namespace internal
{

void
flushCommandBuffer(uint32_t requiredNumWords,
                   BOOL a2);

void
allocateCommandBuffer(uint32_t requiredNumWords);

void
padCommandBuffer(virt_ptr<ActiveCommandBuffer> cb);


/**
 * Get the active command buffer for the current core.
 */
virt_ptr<ActiveCommandBuffer>
getActiveCommandBuffer()
{
   return virt_addrof(sCbPoolData->activeCommandBuffer[cpu::this_core::id()]);
}


/**
 * Get an active command buffer with space to write numWords.
 */
virt_ptr<ActiveCommandBuffer>
getWriteCommandBuffer(uint32_t numWords)
{
   auto cb = getActiveCommandBuffer();
   if (cb->bufferPosWords + numWords > cb->bufferSizeWords) {
      flushCommandBuffer(numWords, TRUE);
   }

   cb->writeGatherPtr = cb->buffer + cb->bufferPosWords;
   cb->bufferPosWords += numWords;
   cb->cmdSizeTarget = numWords;
   cb->cmdSize = 0u;
   return cb;
}


/**
 * Initialise command buffer pool.
 */
void
initialiseCommandBufferPool(virt_ptr<void> base,
                            uint32_t size)
{
   auto &mainCoreCb = sCbPoolData->mainCoreCommandBuffer;
   mainCoreCb.isUserBuffer = FALSE;
   mainCoreCb.cbPoolBase = virt_cast<uint32_t *>(base);
   mainCoreCb.cbPoolNumWords = size / 4;
   mainCoreCb.buffer = virt_cast<uint32_t *>(base);
   mainCoreCb.bufferPosWords = 0u;

   sCbPoolData->activeCommandBuffer[getMainCoreId()] = mainCoreCb;
   sCbPoolData->gpuLastReadCommandBuffer =
      virt_cast<virt_addr>(mainCoreCb.cbPoolBase + mainCoreCb.cbPoolNumWords);

   // Allocate initial command buffer
   allocateCommandBuffer(MinimumCommandBufferNumWords);
}


/**
 * Initialise a command buffer.
 */
void
initialiseCommandBuffer(virt_ptr<ActiveCommandBuffer> cb,
                        virt_ptr<uint32_t> buffer,
                        uint32_t bufferSizeWords,
                        BOOL isUserBuffer)
{
   // Normally this would be set to the write gather address, but we are faking
   // write gathering...!
   cb->writeGatherPtr = buffer;

   cb->isUserBuffer = isUserBuffer;
   cb->buffer = buffer;
   cb->bufferPosWords = 0u;
   cb->bufferSizeWords = bufferSizeWords;
   cb->cmdSize = 0u;
   cb->cmdSizeTarget = 0u;
}


/**
 * Allocate a command buffer with space for requiredNumWords.
 */
void
allocateCommandBuffer(uint32_t requiredNumWords)
{
   auto coreId = cpu::this_core::id();
   auto cb = virt_addrof(sCbPoolData->activeCommandBuffer[cpu::this_core::id()]);
   decaf_check(coreId == getMainCoreId());
   decaf_check(requiredNumWords < MaximumCommandBufferNumWords);
   requiredNumWords = std::max(requiredNumWords, MinimumCommandBufferNumWords);

   // Ring buffer between write pointer and read pointer
   auto writePosition = virt_cast<virt_addr>(cb->buffer + cb->bufferPosWords);
   auto readPosition = virt_addr { sCbPoolData->gpuLastReadCommandBuffer };
   auto ringBufferStart = virt_cast<virt_addr>(cb->cbPoolBase);
   auto ringBufferEnd = virt_cast<virt_addr>(cb->cbPoolBase + cb->cbPoolNumWords);
   auto freeBytes = ptrdiff_t { 0 };
   auto requiredBytes = requiredNumWords * 4;

#ifdef GX2_ENABLE_CBPOOL_TIMEOUT
   auto allocateStartTime = OSGetSystemTime();
   auto gpuTimeoutTicks = coreinit::internal::msToTicks(GX2GetGPUTimeout());
#endif

   while (freeBytes < requiredBytes) {
      auto retiredTimestamp = GX2GetRetiredTimeStamp();

      if (writePosition < readPosition) {
         freeBytes = readPosition - writePosition;
      } else {
         // When writePosition == readPosition, we check the retired and
         // submitted timestamp to decide if the ringbuffer is empty or full.
         if (writePosition == readPosition &&
             GX2GetLastSubmittedTimeStamp() < retiredTimestamp) {
            // Ringbuffer is full
            freeBytes = 0;
         } else if (ringBufferEnd - writePosition >= requiredBytes) {
            freeBytes = ringBufferEnd - writePosition;
         } else {
            // Not enough space at end of the ringbuffer, try from the start instead
            freeBytes = readPosition - ringBufferStart;
            writePosition = ringBufferStart;
         }
      }

      if (freeBytes < requiredBytes) {
#ifdef GX2_ENABLE_CBPOOL_TIMEOUT
         // Check if we have hit timeout waiting for gpu
         if (OSGetSystemTime() - allocateStartTime >= gpuTimeoutTicks) {
            // TODO: Handle GPU timeout
            break;
         }
#endif

         // Wait for next retired buffer to try get some more free pool space
         GX2WaitTimeStamp(retiredTimestamp + 1);
      }
   }

   decaf_check(freeBytes >= requiredBytes);
   auto allocNumWords = std::min<uint32_t>(MaximumCommandBufferNumWords,
                                           static_cast<uint32_t>(freeBytes / 4));
   initialiseCommandBuffer(cb,
                           virt_cast<uint32_t *>(writePosition),
                           allocNumWords,
                           FALSE);
}


/**
 * Begin a user command buffer (aka a display list).
 */
void
beginUserCommandBuffer(virt_ptr<uint32_t> displayList,
                       uint32_t bytes,
                       BOOL profilingEnabled)
{
   auto coreId = cpu::this_core::id();
   auto cb = virt_addrof(sCbPoolData->activeCommandBuffer[cpu::this_core::id()]);
   decaf_check(!cb->isUserBuffer);

   if (coreId == getMainCoreId()) {
      padCommandBuffer(cb);
      sCbPoolData->mainCoreCommandBuffer = *cb;
      sCbPoolData->profilingWasEnabledBeforeUserDisplayList = getProfilingEnabled();
   }

   setProfilingEnabled(profilingEnabled);
   initialiseCommandBuffer(cb, displayList, bytes / 4, TRUE);
}


/**
 * End a user command buffer.
 */
uint32_t
endUserCommandBuffer(virt_ptr<uint32_t> displayList)
{
   auto coreId = cpu::this_core::id();
   auto cb = virt_addrof(sCbPoolData->activeCommandBuffer[cpu::this_core::id()]);
   decaf_check(cb->isUserBuffer);

   // Pad the command buffer so it is ready for submission
   padCommandBuffer(cb);
   auto bufferSize = cb->bufferPosWords * 4;

   if (coreId == getMainCoreId()) {
      // Restore the main command buffer
      *cb = sCbPoolData->mainCoreCommandBuffer;
      setProfilingEnabled(sCbPoolData->profilingWasEnabledBeforeUserDisplayList);
   } else {
      // Clear the user display list
      cb->isUserBuffer = FALSE;
      cb->bufferPosWords = 0u;
      cb->bufferSizeWords = 0u;
      cb->buffer = nullptr;
   }

   return bufferSize;
}


/**
 * Pad a command buffer with NOPs to align to 8 words.
 */
void
padCommandBuffer(virt_ptr<ActiveCommandBuffer> cb)
{
   auto alignedBufferPos = align_up(cb->bufferPosWords, 8);
   if (alignedBufferPos != cb->bufferPosWords) {
      auto padNumWords = alignedBufferPos - cb->bufferPosWords;
      cb = getWriteCommandBuffer(padNumWords);

      for (auto i = 0u; i < padNumWords; ++i) {
         *cb->writeGatherPtr = 0x80000000u;
      }
   }
}


/**
 * Queue a command buffer in the gpu ring buffer.
 */
void
queueCommandBuffer(virt_ptr<uint32_t> cbBase,
                   uint32_t cbSize,
                   virt_ptr<virt_addr> gpuLastReadPointer,
                   BOOL writeConfirmTimestamp)
{
   auto submitCommand = StackArray<uint32_t, 9> { };
   auto submitCommandNumWords = uint32_t { 0u };

   decaf_check(cbBase);
   decaf_check(cbSize);

   auto indirectBufferCall =
      latte::pm4::IndirectBufferCallPriv {
         OSEffectiveToPhysical(virt_cast<virt_addr>(cbBase)),
         cbSize
   };
   writePM4(submitCommand, submitCommandNumWords, indirectBufferCall);

   if (gpuLastReadPointer) {
      auto memWrite =
         latte::pm4::MemWrite {
            latte::pm4::MW_ADDR_LO::get(0)
               .ADDR_LO(OSEffectiveToPhysical(virt_cast<virt_addr>(gpuLastReadPointer)) >> 2)
               .ENDIAN_SWAP(latte::CB_ENDIAN::SWAP_8IN32),
            latte::pm4::MW_ADDR_HI::get(0)
               .CNTR_SEL(latte::pm4::MW_WRITE_DATA)
               .DATA32(true),
            static_cast<uint32_t>(virt_cast<virt_addr>(cbBase + cbSize)),
            0u
      };
      writePM4(submitCommand, submitCommandNumWords, memWrite);
   }

   if (getProfileMode() & GX2ProfileMode::SkipExecuteCommandBuffers) {
      // Change IndirectBufferCallPriv to a NOP
      submitCommand[0] =
         latte::pm4::HeaderType3::get(0)
         .type(latte::pm4::PacketType::Type3)
         .opcode(latte::pm4::IT_OPCODE::NOP)
         .size(2)
         .value;
   }

   // TODO: Call pm4 capture flush command buffer

   auto submitFlags = StackObject<TCLSubmitFlags> { };
   *submitFlags = TCLSubmitFlags::UpdateTimestamp;
   if (!writeConfirmTimestamp) {
      *submitFlags |= TCLSubmitFlags::NoWriteConfirmTimestamp;
   }

   captureCommandBuffer(submitCommand,
                        submitCommandNumWords);

   if (!debugCaptureEnabled()) {
      TCLSubmitToRing(submitCommand,
                      submitCommandNumWords,
                      submitFlags,
                      virt_addrof(sCbPoolData->lastSubmittedTimestamp));
   } else {
      debugCaptureSubmit(submitCommand,
                         submitCommandNumWords,
                         submitFlags,
                         virt_addrof(sCbPoolData->lastSubmittedTimestamp));
   }
}


/**
 * Flush the currently active command buffer.
 */
void
flushCommandBuffer(uint32_t requiredNumWords,
                   BOOL writeConfirmTimestamp)
{
   auto coreId = cpu::this_core::id();
   auto cb = virt_addrof(sCbPoolData->activeCommandBuffer[cpu::this_core::id()]);

   if (cb->isUserBuffer) {
      auto usedSize = GX2EndDisplayList(cb->buffer);
      auto newSize = 4 * requiredNumWords;
      auto newDisplayList = displayListOverrun(cb->buffer, usedSize, newSize);

      GX2BeginDisplayListEx(newDisplayList.first,
                            newDisplayList.second,
                            getProfilingEnabled());
   } else {
      decaf_check(coreId == getMainCoreId());

      if (cb->bufferPosWords != 0) {
         padCommandBuffer(cb);
         queueCommandBuffer(cb->buffer,
                            cb->bufferPosWords,
                            virt_addrof(sCbPoolData->gpuLastReadCommandBuffer),
                            writeConfirmTimestamp);
      }

      allocateCommandBuffer(requiredNumWords);
      decaf_check(requiredNumWords <= cb->bufferSizeWords - cb->bufferPosWords);
   }
}


/**
 * Inform debug capture about the pointers that we send over PM4 for commands
 * such as EVENT_WRITE_EOP.
 */
void
debugCaptureCbPoolPointers()
{
   debugCaptureAlloc(virt_addrof(sCbPoolData->gpuLastReadCommandBuffer),
                     sizeof(sCbPoolData->gpuLastReadCommandBuffer), 8);

   debugCaptureAlloc(virt_addrof(sCbPoolData->lastSubmittedTimestamp),
                     sizeof(sCbPoolData->lastSubmittedTimestamp), 8);

   debugCaptureInvalidate(virt_addrof(sCbPoolData->gpuLastReadCommandBuffer),
                          sizeof(sCbPoolData->gpuLastReadCommandBuffer));

   debugCaptureInvalidate(virt_addrof(sCbPoolData->lastSubmittedTimestamp),
                          sizeof(sCbPoolData->lastSubmittedTimestamp));
}

void
debugCaptureCbPoolPointersFree()
{
   debugCaptureFree(virt_addrof(sCbPoolData->gpuLastReadCommandBuffer));
   debugCaptureFree(virt_addrof(sCbPoolData->lastSubmittedTimestamp));
}

} // namespace internal

void
Library::registerCbPoolSymbols()
{
   RegisterFunctionExport(GX2GetLastSubmittedTimeStamp);
   RegisterFunctionExport(GX2GetRetiredTimeStamp);
   RegisterFunctionExport(GX2SubmitUserTimeStamp);
   RegisterFunctionExport(GX2WaitTimeStamp);

   RegisterDataInternal(sCbPoolData);
}

} // namespace cafe::gx2
