#include "cafe_kernel_loader.h"
#include "cafe_kernel_mmu.h"
#include "cafe_kernel_process.h"

#include "cafe/loader/cafe_loader_entry.h"
#include "cafe/loader/cafe_loader_globals.h"
#include "cafe/loader/cafe_loader_loaded_rpl.h"

#include <cstdint>
#include <libcpu/cpu.h>

namespace cafe::kernel
{

namespace internal
{
static int32_t loaderEntry();
} // namespace internal


int32_t
loaderLink(loader::LOADER_Handle handle,
           virt_ptr<loader::LOADER_MinFileInfo> minFileInfo,
           virt_ptr<loader::LOADER_LinkInfo> linkInfo,
           uint32_t linkInfoSize)
{
   auto loaderIpc = loader::getKernelIpcStorage();
   loaderIpc->entryParams.dispatch.code = loader::LOADER_Code::Link;
   loaderIpc->entryParams.dispatch.handle = handle;
   loaderIpc->entryParams.dispatch.minFileInfo = minFileInfo;
   loaderIpc->entryParams.dispatch.linkInfo = linkInfo;
   loaderIpc->entryParams.dispatch.linkInfoSize = linkInfoSize;
   return internal::loaderEntry();
}

int32_t
loaderPrep(virt_ptr<loader::LOADER_MinFileInfo> minFileInfo)
{
   auto loaderIpc = loader::getKernelIpcStorage();
   loaderIpc->entryParams.dispatch.code = loader::LOADER_Code::Prep;
   loaderIpc->entryParams.dispatch.minFileInfo = minFileInfo;
   return internal::loaderEntry();
}

int32_t
loaderPurge(loader::LOADER_Handle handle)
{
   auto loaderIpc = loader::getKernelIpcStorage();
   loaderIpc->entryParams.dispatch.code = loader::LOADER_Code::Purge;
   loaderIpc->entryParams.dispatch.handle = handle;
   return internal::loaderEntry();
}

int32_t
loaderSetup(loader::LOADER_Handle handle,
            virt_ptr<loader::LOADER_MinFileInfo> minFileInfo)
{
   auto loaderIpc = loader::getKernelIpcStorage();
   loaderIpc->entryParams.dispatch.code = loader::LOADER_Code::Setup;
   loaderIpc->entryParams.dispatch.handle = handle;
   loaderIpc->entryParams.dispatch.minFileInfo = minFileInfo;
   return internal::loaderEntry();
}

int32_t
loaderQuery(loader::LOADER_Handle handle,
            virt_ptr<loader::LOADER_MinFileInfo> outMinFileInfo)
{
   auto loaderIpc = loader::getKernelIpcStorage();
   loaderIpc->entryParams.dispatch.code = loader::LOADER_Code::Query;
   loaderIpc->entryParams.dispatch.handle = handle;
   loaderIpc->entryParams.dispatch.minFileInfo = outMinFileInfo;
   return internal::loaderEntry();
}

int32_t
loaderUserGainControl()
{
   auto loaderIpc = loader::getKernelIpcStorage();
   loaderIpc->entryParams.dispatch.code = loader::LOADER_Code::UserGainControl;
   return internal::loaderEntry();
}

int32_t
findClosestSymbol(virt_addr addr,
                  virt_ptr<uint32_t> outSymbolDistance,
                  virt_ptr<char> symbolNameBuffer,
                  uint32_t symbolNameBufferLength,
                  virt_ptr<char> moduleNameBuffer,
                  uint32_t moduleNameBufferLength)
{
   // Verify parameters
   if (outSymbolDistance &&
      (virt_cast<virt_addr>(outSymbolDistance) < virt_addr { 0x10000000 } ||
       virt_cast<virt_addr>(outSymbolDistance) >= virt_addr { 0xC0000000 } ||
       virt_cast<virt_addr>(outSymbolDistance) & 3 ||
       !validateAddressRange(virt_cast<virt_addr>(outSymbolDistance), 4))) {
      return 0xBAD20008;
   }

   if (symbolNameBufferLength) {
      if (virt_cast<virt_addr>(symbolNameBuffer) < virt_addr { 0x10000000 } ||
          virt_cast<virt_addr>(symbolNameBuffer) >= virt_addr { 0xC0000000 } ||
          !validateAddressRange(virt_cast<virt_addr>(symbolNameBuffer), symbolNameBufferLength)) {
          return 0xBAD2000A;
      }
   } else {
      symbolNameBuffer = nullptr;
   }

   if (moduleNameBufferLength) {
      if (virt_cast<virt_addr>(moduleNameBuffer) < virt_addr { 0x10000000 } ||
          virt_cast<virt_addr>(moduleNameBuffer) >= virt_addr { 0xC0000000 } ||
          !validateAddressRange(virt_cast<virt_addr>(moduleNameBuffer), moduleNameBufferLength)) {
       return 0xBAD2000A;
      }
   } else {
      moduleNameBuffer = nullptr;
   }

   return internal::findClosestSymbol(addr,
                                      outSymbolDistance,
                                      symbolNameBuffer,
                                      symbolNameBufferLength,
                                      moduleNameBuffer,
                                      moduleNameBufferLength);
}

namespace internal
{

static std::pair<virt_ptr<Context>, virt_addr>
getLoaderContext()
{
   auto contextStorage = loader::getContextStorage();
   auto context = virt_ptr<Context> { nullptr };
   auto stackTop = virt_addr { 0 };

   switch (cpu::this_core::id()) {
   case 0:
      context = virt_addrof(contextStorage->context0);
      stackTop = virt_cast<virt_addr>(virt_addrof(contextStorage->stack0) + contextStorage->stack0.size() - 16);
      break;
   case 1:
      context = virt_addrof(contextStorage->context1);
      stackTop = virt_cast<virt_addr>(virt_addrof(contextStorage->stack1) + contextStorage->stack1.size() - 16);
      break;
   case 2:
      context = virt_addrof(contextStorage->context2);
      stackTop = virt_cast<virt_addr>(virt_addrof(contextStorage->stack2) + contextStorage->stack2.size() - 16);
      break;
   default:
      decaf_abort(fmt::format("Unexpected core id {}", cpu::this_core::id()));
   }

   return { context, stackTop };
}

static int32_t
loaderEntry()
{
   auto loaderIpc = loader::getKernelIpcStorage();
   auto [context, stackTop] = getLoaderContext();

   loaderIpc->entryParams.context = context;
   loaderIpc->entryParams.procConfig = 0;
   loaderIpc->entryParams.procContext = getCurrentContext();
   loaderIpc->entryParams.interruptsAllowed = TRUE;
   loaderIpc->entryParams.procId = getCurrentUniqueProcessId();

   // In a real kernel we'd switch to the PPC context
   // But our loader is HLE only atm so we just call the loader directly
   return loader::LoaderStart(TRUE, virt_addrof(loaderIpc->entryParams));
}

static void
KiRPLLoaderSetup(ProcessFlags processFlags,
                 UniqueProcessId callerProcessId,
                 UniqueProcessId targetProcessId)
{
   auto loaderIpc = loader::getKernelIpcStorage();
   auto [context, stackTop] = getLoaderContext();

   if (targetProcessId == UniqueProcessId::Root) {
      processFlags = ProcessFlags::get(0).
         isFirstProcess(true);
   } else {
      // Clear all flags except debugLevel
      processFlags = ProcessFlags::get(0).
         debugLevel(processFlags.debugLevel());

      if (processFlags.unkBit12() || processFlags.isFirstProcess()) {
         processFlags = processFlags
            .disableSharedLibraries(true);
      }

      // TODO: Fix this when we implement multi-process
      // Normally this would only be set by Root process, but in decaf until we
      // implement processes then we will always be first process
      processFlags = processFlags.
         isFirstProcess(true);
   }

   loaderIpc->unk0x00 = 0u;
   loaderIpc->processFlags = processFlags;

   // In a real kernel we'd switch to the PPC context
   // context->srr0 = loader entry point
   // context->srr1 = 0x4000
   context->gpr[1] = static_cast<uint32_t>(stackTop);
   context->gpr[3] = 0u;
   context->gpr[4] = static_cast<uint32_t>(virt_cast<virt_addr>(virt_addrof(loaderIpc->entryParams)));

   // But our loader is HLE only atm so we just call the loader directly
   loader::LoaderStart(FALSE, virt_addrof(loaderIpc->entryParams));
}

void
KiRPLStartup(UniqueProcessId callerProcessId,
             UniqueProcessId targetProcessId,
             ProcessFlags processFlags,
             uint32_t numCodeAreaHeapBlocks,
             uint32_t maxCodeSize,
             uint32_t maxDataSize,
             uint32_t titleLoc)
{
   auto loaderIpc = loader::getKernelIpcStorage();
   loaderIpc->maxDataSize = maxDataSize;
   loaderIpc->callerProcessId = callerProcessId;
   loaderIpc->maxCodeSize = maxCodeSize;
   loaderIpc->procTitleLoc = titleLoc;
   loaderIpc->targetProcessId = targetProcessId;
   loaderIpc->numCodeAreaHeapBlocks = numCodeAreaHeapBlocks;
   loaderIpc->rpxModule = nullptr;
   loaderIpc->loadedModuleList = nullptr;
   loaderIpc->unk0x28 = 0u;

   loaderIpc->entryParams.procContext = nullptr;
   loaderIpc->entryParams.procId = UniqueProcessId::Invalid;
   loaderIpc->entryParams.procConfig = -1;
   loaderIpc->entryParams.context = nullptr;
   loaderIpc->entryParams.interruptsAllowed = FALSE;
   std::memset(virt_addrof(loaderIpc->entryParams.dispatch).getRawPointer(),
               0, sizeof(loader::LOADER_EntryDispatch));

   KiRPLLoaderSetup(processFlags, callerProcessId, targetProcessId);
}

int32_t
findClosestSymbol(virt_addr addr,
                  uint32_t *outSymbolDistance,
                  char *symbolNameBuffer,
                  uint32_t symbolNameBufferLength,
                  char *moduleNameBuffer,
                  uint32_t moduleNameBufferLength)
{
   auto partitionData = getCurrentRamPartitionData();
   if (!partitionData) {
      return 0xBAD20002;
   }

   if (outSymbolDistance) {
      *outSymbolDistance = static_cast<uint32_t>(addr);
   }

   if (symbolNameBuffer) {
      symbolNameBuffer[0] = char { 0 };
   }

   if (moduleNameBuffer) {
      moduleNameBuffer[0] = char { 0 };
   }

   // Find the module and section containing the given address
   auto containingModule = virt_ptr<loader::LOADED_RPL> { nullptr };
   auto containingSectionIndex = 0u;

   for (auto rpl = partitionData->loadedModuleList; rpl; rpl = rpl->nextLoadedRpl) {
      for (auto i = 0u; i < rpl->elfHeader.shnum; ++i) {
         auto sectionAddress = rpl->sectionAddressBuffer[i];
         if (!sectionAddress) {
            continue;
         }

         auto sectionHeader =
            virt_cast<loader::rpl::SectionHeader *>(
               virt_cast<virt_addr>(rpl->sectionHeaderBuffer) +
               rpl->elfHeader.shentsize * i);

         if (addr >= sectionAddress &&
             addr < sectionAddress + sectionHeader->size) {
            containingModule = rpl;
            containingSectionIndex = i;
            break;
         }
      }
   }

   // Search the module's symbol table for the nearest symbol
   auto nearestSymbolName = virt_ptr<const char> { nullptr };
   auto nearestSymbolValue = virt_addr { 0 };

   if (containingModule) {
      auto nearestSymbolDistance = uint32_t { 0xFFFFFFFFu };
      for (auto i = 0u; i < containingModule->elfHeader.shnum; ++i) {
         auto sectionAddress = containingModule->sectionAddressBuffer[i];
         auto sectionHeader =
            virt_cast<loader::rpl::SectionHeader *>(
               virt_cast<virt_addr>(containingModule->sectionHeaderBuffer) +
               containingModule->elfHeader.shentsize * i);

         if (sectionHeader->type == loader::rpl::SHT_SYMTAB) {
            auto strTab = containingModule->sectionAddressBuffer[sectionHeader->link];
            auto symTabEntSize =
               sectionHeader->entsize ?
               static_cast<uint32_t>(sectionHeader->entsize) :
               sizeof(loader::rpl::Symbol);
            auto numSymbols = sectionHeader->size / symTabEntSize;

            for (auto j = 0u; j < numSymbols; ++j) {
               auto symbol =
                  virt_cast<loader::rpl::Symbol *>(sectionAddress + j * symTabEntSize);
               auto symbolAddr = virt_addr { static_cast<uint32_t>(symbol->value) };
               if (symbol->shndx != containingSectionIndex ||
                   symbolAddr > addr) {
                  continue;
               }

               // Ignore symbols beginning with $ or .
               auto symbolName = virt_cast<const char *>(strTab + symbol->name);
               if (symbolName[0] == '$' || symbolName[0] == '.') {
                  continue;
               }

               if (addr - symbolAddr < nearestSymbolDistance) {
                  nearestSymbolName = symbolName;
                  nearestSymbolDistance = static_cast<uint32_t>(addr - symbolAddr);
                  nearestSymbolValue = symbolAddr;

                  if (nearestSymbolDistance == 0) {
                     break;
                  }
               }
            }

            if (nearestSymbolDistance == 0) {
               break;
            }
         }
      }
   }

   // Set the output
   if (moduleNameBuffer) {
      if (containingModule) {
         std::strncpy(moduleNameBuffer,
                      containingModule->moduleNameBuffer.getRawPointer(),
                      moduleNameBufferLength);
      } else {
         moduleNameBuffer[0] = char { 0 };
      }
   }

   if (symbolNameBuffer) {
      if (nearestSymbolName && nearestSymbolName[0]) {
         std::strncpy(symbolNameBuffer,
                      nearestSymbolName.getRawPointer(),
                      symbolNameBufferLength);
      } else {
         std::strncpy(symbolNameBuffer,
                      "<unknown>",
                      symbolNameBufferLength);
      }
   }

   if (outSymbolDistance) {
      *outSymbolDistance = static_cast<uint32_t>(addr - nearestSymbolValue);
   }

   return 0;
}

int32_t
findClosestSymbol(virt_addr addr,
                  virt_ptr<uint32_t> outSymbolDistance,
                  virt_ptr<char> symbolNameBuffer,
                  uint32_t symbolNameBufferLength,
                  virt_ptr<char> moduleNameBuffer,
                  uint32_t moduleNameBufferLength)
{
   auto symbolDistance = uint32_t { 0 };
   auto result = findClosestSymbol(addr,
                                   &symbolDistance,
                                   symbolNameBuffer.getRawPointer(),
                                   symbolNameBufferLength,
                                   moduleNameBuffer.getRawPointer(),
                                   moduleNameBufferLength);
   *outSymbolDistance = symbolDistance;
   return result;
}

} // namespace internal

} // namespace cafe::kernel::internal
