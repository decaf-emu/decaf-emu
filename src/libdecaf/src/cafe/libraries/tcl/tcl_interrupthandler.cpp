#include "tcl.h"
#include "tcl_enum.h"
#include "tcl_interrupthandler.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_interrupts.h"

#include <libcpu/be2_atomic.h>
#include <libcpu/be2_struct.h>
#include <libgpu/gpu_ih.h>

namespace cafe::tcl
{

using namespace cafe::coreinit;

constexpr auto MaxNumInterruptTypes = 256u;
constexpr auto MaxNumHandlersPerInterrupt = 4u;

struct RegisteredInterruptHandler
{
   be2_val<TCLInterruptHandlerFn> callback;
   be2_virt_ptr<void> userData;
};

struct StaticInterruptHandlerData
{
   be2_array<be2_array<RegisteredInterruptHandler, MaxNumHandlersPerInterrupt>, MaxNumInterruptTypes> handlers;

   be2_atomic<uint32_t> interruptCount;
   be2_atomic<uint32_t> interruptEntriesRead;
   std::array<be2_atomic<uint32_t>, 0x100> interruptTypeCount;
};

static virt_ptr<StaticInterruptHandlerData>
sInterruptHandlerData = nullptr;

static OSUserInterruptHandler
sInterruptHandler = nullptr;

TCLStatus
TCLIHEnableInterrupt(TCLInterruptType type,
                     BOOL enable)
{
   auto cp_int_cntl = latte::CP_INT_CNTL::get(0);
   switch (type) {
   case TCLInterruptType::UNKNOWN_192:
      cp_int_cntl = cp_int_cntl.UNK17_INT_ENABLE(true);
      break;
   case TCLInterruptType::CP_RB:
      cp_int_cntl = cp_int_cntl.RB_INT_ENABLE(true);
      break;
   case TCLInterruptType::CP_IB1:
      cp_int_cntl = cp_int_cntl.IB1_INT_ENABLE(true);
      break;
   case TCLInterruptType::CP_IB2:
      cp_int_cntl = cp_int_cntl.IB2_INT_ENABLE(true);
      break;
   case TCLInterruptType::CP_RESERVED_BITS:
      cp_int_cntl = cp_int_cntl.RESERVED_BITS_EXCEPTION(true);
      break;
   case TCLInterruptType::CP_EOP_EVENT:
      cp_int_cntl = cp_int_cntl.TIME_STAMP_INT_ENABLE(true);
      break;
   case TCLInterruptType::SCRATCH:
      cp_int_cntl = cp_int_cntl.SCRATCH_INT_ENABLE(true);
      break;
   case TCLInterruptType::CP_BAD_OPCODE:
      cp_int_cntl = cp_int_cntl.BAD_OPCODE_EXCEPTION(true);
      break;
   case TCLInterruptType::CP_CTX_EMPTY:
      cp_int_cntl = cp_int_cntl.CNTX_EMPTY_INT_ENABLE(true);
      break;
   case TCLInterruptType::CP_CTX_BUSY:
      cp_int_cntl = cp_int_cntl.CNTX_BUSY_INT_ENABLE(true);
      break;
   case TCLInterruptType::DMA_CTX_EMPTY:
      // dma_int_cntl.CTXEMPTY_INT_ENABLE(true);
   case TCLInterruptType::DMA_TRAP_EVENT:
      // dma_int_cntl.TRAP_ENABLE(true);
   case TCLInterruptType::DMA_SEM_INCOMPLETE:
      // dma_int_cntl.SEM_INCOMPLETE_INT_ENABLE(true);
   case TCLInterruptType::DMA_SEM_WAIT:
      // dma_int_cntl.SEM_WAIT_INT_ENABLE(true);
   default:
      return TCLStatus::InvalidArg;
   }

   if (enable) {
      gpu::ih::enable(cp_int_cntl);
   } else {
      gpu::ih::disable(cp_int_cntl);
   }

   return TCLStatus::OK;
}

TCLStatus
TCLIHRegister(TCLInterruptType type,
              TCLInterruptHandlerFn callback,
              virt_ptr<void> userData)
{
   if (type >= sInterruptHandlerData->handlers.size()) {
      return TCLStatus::InvalidArg;
   }

   for (auto &handler : sInterruptHandlerData->handlers[type]) {
      if (!handler.callback) {
         handler.callback = callback;
         handler.userData = userData;
         return TCLStatus::OK;
      }
   }

   return TCLStatus::OutOfMemory;
}

TCLStatus
TCLIHUnregister(TCLInterruptType type,
                TCLInterruptHandlerFn callback,
                virt_ptr<void> userData)
{
   if (type >= sInterruptHandlerData->handlers.size()) {
      return TCLStatus::InvalidArg;
   }

   for (auto &handler : sInterruptHandlerData->handlers[type]) {
      if (handler.callback == callback && handler.userData == userData) {
         handler.callback = nullptr;
         handler.userData = nullptr;
         return TCLStatus::OK;
      }
   }

   return TCLStatus::InvalidArg;
}

TCLStatus
TCLGetInterruptCount(TCLInterruptType type,
                     BOOL resetCount,
                     virt_ptr<uint32_t> count)
{
   if (type == 0x100) {
      if (resetCount) {
         *count = sInterruptHandlerData->interruptCount.fetch_and(0);
      } else {
         *count = sInterruptHandlerData->interruptCount.load();
      }

      return TCLStatus::OK;
   }

   if (type == 0x101) {
      if (resetCount) {
         *count = sInterruptHandlerData->interruptEntriesRead.fetch_and(0);
      } else {
         *count = sInterruptHandlerData->interruptEntriesRead.load();
      }

      return TCLStatus::OK;
   }

   if (type < sInterruptHandlerData->handlers.size()) {
      if (resetCount) {
         *count = sInterruptHandlerData->interruptTypeCount[type].fetch_and(0);
      } else {
         *count = sInterruptHandlerData->interruptTypeCount[type].load();
      }

      return TCLStatus::OK;
   }

   return TCLStatus::InvalidArg;
}

namespace internal
{

static void
gpuInterruptHandler(OSInterruptType type,
                    virt_ptr<OSContext> interruptedContext)
{
   auto interruptEntry = StackObject<TCLInterruptEntry> { };
   auto entries = gpu::ih::read();

   sInterruptHandlerData->interruptCount.fetch_add(1);

   for (auto &entry : entries) {
      interruptEntry->interruptSourceID = static_cast<TCLInterruptType>(entry.word0);
      interruptEntry->reservedWord1 = entry.word1;
      interruptEntry->interruptSourceData = entry.word2;
      interruptEntry->reservedWord3 = entry.word3;
      sInterruptHandlerData->interruptEntriesRead.fetch_add(1);

      // Dispatch interrupt entry to registered handlers
      for (auto &handler : sInterruptHandlerData->handlers[entry.word0]) {
         if (handler.callback) {
            cafe::invoke(cpu::this_core::state(),
                         handler.callback,
                         interruptEntry,
                         handler.userData);
         }
      }
   }
}

void
initialiseInterruptHandler()
{
   OSSetInterruptHandler(OSInterruptType::Gpu7, sInterruptHandler);
   gpu::ih::setInterruptCallback([]() { cpu::interrupt(1, cpu::GPU7_INTERRUPT); });
}

} // namespace internal

void
Library::registerInterruptHandlerSymbols()
{
   RegisterFunctionExport(TCLIHEnableInterrupt);
   RegisterFunctionExport(TCLIHRegister);
   RegisterFunctionExport(TCLIHUnregister);
   RegisterFunctionExport(TCLGetInterruptCount);

   RegisterDataInternal(sInterruptHandlerData);
   RegisterFunctionInternal(internal::gpuInterruptHandler,
                            sInterruptHandler);
}

} // namespace cafe::tcl
