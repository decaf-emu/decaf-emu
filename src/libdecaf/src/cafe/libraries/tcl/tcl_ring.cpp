#include "tcl.h"
#include "tcl_interrupthandler.h"
#include "tcl_ring.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_event.h"
#include "cafe/libraries/coreinit/coreinit_memory.h"
#include "cafe/libraries/coreinit/coreinit_time.h"

#include <libcpu/be2_atomic.h>
#include <libgpu/gpu_ringbuffer.h>
#include <libgpu/latte/latte_pm4.h>
#include <libgpu/latte/latte_pm4_commands.h>
#include <libgpu/latte/latte_pm4_sizer.h>
#include <libgpu/latte/latte_pm4_writer.h>

namespace cafe::tcl
{

using namespace latte;
using namespace cafe::coreinit;

struct StaticRingData
{
   be2_array<char, 32> waitCpRetireTimestampEventName;
   be2_struct<OSEvent> waitCpRetireTimestampEvent;
   be2_struct<OSEvent> waitDmaeRetireTimestampEvent;

   be2_atomic<uint64_t> cpSubmitTimestamp;
   be2_atomic<uint64_t> cpRetireTimestamp;
   be2_atomic<uint64_t> dmaeRetireTimestamp;

   //! Physical address of the above cpRetireTimestamp variable.
   be2_val<phys_addr> cpRetireTimestampAddress;
};

static virt_ptr<StaticRingData>
sRingData;

TCLStatus
TCLReadTimestamp(TCLTimestampID id,
                 virt_ptr<TCLTimestamp> outValue)
{
   switch (id) {
   case TCLTimestampID::CPSubmitted:
      *outValue = sRingData->cpSubmitTimestamp.load() - 1;
      break;
   case TCLTimestampID::CPRetired:
      *outValue = sRingData->cpRetireTimestamp.load();
      break;
   case TCLTimestampID::DMAERetired:
      *outValue = sRingData->dmaeRetireTimestamp.load();
      break;
   default:
      return TCLStatus::InvalidArg;
   }

   return TCLStatus::OK;
}

TCLStatus
TCLWaitTimestamp(TCLTimestampID id,
                 TCLTimestamp timestamp,
                 OSTime timeout)
{
   auto endTime = OSGetSystemTime() + timeout;
   if (id == TCLTimestampID::CPRetired) {
      while (timestamp > sRingData->cpRetireTimestamp.load()) {
         if (OSGetSystemTime() >= endTime) {
            return TCLStatus::Timeout;
         }

         OSWaitEventWithTimeout(virt_addrof(sRingData->waitCpRetireTimestampEvent),
                                500000);
      }
   } else {
      return TCLStatus::InvalidArg;
   }

   return TCLStatus::OK;
}

TCLStatus
TCLSubmit(phys_ptr<void> buffer,
          uint32_t bufferSize,
          virt_ptr<TCLSubmitFlags> submitFlags,
          virt_ptr<TCLTimestamp> lastSubmittedTimestamp)
{
   return TCLStatus::NotInitialised;
}

template<typename Type>
void
writePM4(uint32_t *buffer,
         uint32_t &bufferPosWords,
         const Type &command)
{
   // Remove const for the .serialise function
   auto &cmd = const_cast<Type &>(command);

   // Calculate the total size this object will be
   latte::pm4::PacketSizer sizer;
   cmd.serialise(sizer);
   auto totalSize = sizer.getSize() + 1;

   // Serialize the packet to the given buffer
   auto writer = latte::pm4::PacketWriter {
      buffer,
      bufferPosWords,
      Type::Opcode,
      totalSize
   };
   cmd.serialise(writer);
}

static TCLTimestamp
insertRetiredTimestamp(bool cacheFlushTimestamp,
                       bool cacheFlushInvalidate,
                       bool writeConfirm)
{
   auto submitTimestamp = sRingData->cpSubmitTimestamp.fetch_add(1);
   auto eventType = VGT_EVENT_TYPE::BOTTOM_OF_PIPE_TS;
   if (cacheFlushTimestamp) {
      if (cacheFlushInvalidate) {
         eventType = VGT_EVENT_TYPE::CACHE_FLUSH_AND_INV_TS_EVENT;
      } else {
         eventType = VGT_EVENT_TYPE::CACHE_FLUSH_TS;
      }
   }

   // Submit an EVENT_WRITE_EOP to update the retire timestamp
   std::array<uint32_t, 6> buffer;
   auto bufferPos = 0u;
   writePM4(buffer.data(), bufferPos,
      pm4::EventWriteEOP {
         VGT_EVENT_INITIATOR::get(0)
            .EVENT_TYPE(eventType)
            .EVENT_INDEX(VGT_EVENT_INDEX::TS),
         pm4::EW_ADDR_LO::get(0)
            .ADDR_LO(static_cast<uint32_t>(sRingData->cpRetireTimestampAddress) >> 2)
            .ENDIAN_SWAP(CB_ENDIAN::SWAP_8IN64),
         pm4::EWP_ADDR_HI::get(0)
            .DATA_SEL(pm4::EWP_DATA_64)
            .INT_SEL(writeConfirm ? pm4::EWP_INT_WRITE_CONFIRM : pm4::EWP_INT_NONE),
         static_cast<uint32_t>(submitTimestamp & 0xFFFFFFFF),
         static_cast<uint32_t>(submitTimestamp >> 32)
      });
   gpu::ringbuffer::write(buffer);
   return submitTimestamp;
}

TCLStatus
TCLSubmitToRing(virt_ptr<uint32_t> buffer,
                uint32_t numWords,
                virt_ptr<TCLSubmitFlags> submitFlags,
                virt_ptr<TCLTimestamp> lastSubmittedTimestamp)
{
   auto flags = TCLSubmitFlags::None;
   auto submitTimestamp = TCLTimestamp { 0 };
   if (submitFlags) {
      flags = *submitFlags;
   }

   gpu::ringbuffer::write({ buffer.getRawPointer(), numWords });

   if (flags & TCLSubmitFlags::UpdateTimestamp) {
      submitTimestamp =
         insertRetiredTimestamp(!(flags & TCLSubmitFlags::NoCacheFlush),
                                !!(flags & TCLSubmitFlags::CacheFlushInvalidate),
                                !(flags & TCLSubmitFlags::NoWriteConfirmTimestamp));
   } else {
      submitTimestamp = sRingData->cpSubmitTimestamp.load();
   }

   if (lastSubmittedTimestamp) {
      *lastSubmittedTimestamp = submitTimestamp;
   }

   return TCLStatus::OK;
}

namespace internal
{

static TCLInterruptHandlerFn sCpEopEventCallback = nullptr;

static void
cpEopEventCallback(virt_ptr<TCLInterruptEntry> interruptEntry,
                   virt_ptr<void> userData)
{
   OSSignalEvent(virt_addrof(sRingData->waitCpRetireTimestampEvent));
}

void
initialiseRing()
{
   // TODO: be2_atomic virt_addrof
   sRingData->cpRetireTimestampAddress =
      coreinit::OSEffectiveToPhysical(cpu::translate(&sRingData->cpRetireTimestamp));

   sRingData->waitCpRetireTimestampEventName = "{ GX2 CP Retire }";
   OSInitEventEx(virt_addrof(sRingData->waitCpRetireTimestampEvent),
                 FALSE,
                 OSEventMode::AutoReset,
                 virt_addrof(sRingData->waitCpRetireTimestampEventName));

   TCLIHRegister(TCLInterruptType::CP_EOP_EVENT, sCpEopEventCallback, nullptr);

   // tcl.rpl also register these but only do a COSWarn in the callbacks, nothing interesting.
   // TCLIHRegister(TCLInterruptType::CP_RESERVED_BITS, sCpReservedBitsException, nullptr);
   // TCLIHRegister(TCLInterruptType::CP_BAD_OPCODE, sCpBadOpcodeException, nullptr);
}

} // namespace internal

void
Library::registerRingSymbols()
{
   RegisterFunctionExport(TCLReadTimestamp);
   RegisterFunctionExport(TCLWaitTimestamp);
   RegisterFunctionExport(TCLSubmit);
   RegisterFunctionExport(TCLSubmitToRing);

   RegisterDataInternal(sRingData);
   RegisterFunctionInternal(internal::cpEopEventCallback,
                            internal::sCpEopEventCallback);

}

} // namespace cafe::tcl
