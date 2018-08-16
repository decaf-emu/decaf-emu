#include "cafe_kernel_info.h"
#include "cafe_kernel_process.h"
#include "cafe/loader/cafe_loader_init.h"
#include "cafe/loader/cafe_loader_loaded_rpl.h"

namespace cafe::kernel
{

void
getType0Info(virt_ptr<Info0> info,
             uint32_t size)
{
   info->upid = getCurrentUpid();
   info->rampid = getCurrentRampid();
   info->appFlags = ProcessFlags::get(0)
      .debugLevel(DebugLevel::Verbose)
      .disableSharedLibraries(false)
      .isFirstProcess(true);

   auto startInfo = getProcessStartInfo();
   info->dataAreaStart = startInfo->dataAreaStart;
   info->dataAreaEnd = startInfo->dataAreaEnd;
   info->sdaBase = startInfo->sdaBase;
   info->sda2Base = startInfo->sda2Base;
   info->systemHeapSize = startInfo->systemHeapSize;

   auto processData = getCurrentProcessData();
   auto &core0 = processData->perCoreStartInfo[0];
   auto &core1 = processData->perCoreStartInfo[1];
   auto &core2 = processData->perCoreStartInfo[2];

   info->stackBase0 = core0.stackBase;
   info->stackBase1 = core1.stackBase;
   info->stackBase2 = core2.stackBase;

   info->stackEnd0 = core0.stackEnd;
   info->stackEnd1 = core1.stackEnd;
   info->stackEnd2 = core2.stackEnd;

   info->exceptionStackBase0 = core0.exceptionStackBase;
   info->exceptionStackBase1 = core1.exceptionStackBase;
   info->exceptionStackBase2 = core2.exceptionStackBase;

   info->exceptionStackEnd0 = core0.exceptionStackEnd;
   info->exceptionStackEnd1 = core1.exceptionStackEnd;
   info->exceptionStackEnd2 = core2.exceptionStackEnd;

   info->lockedCacheBase0 = virt_addr { 0xFFC00000 };
   info->lockedCacheBase1 = virt_addr { 0xFFC40000 };
   info->lockedCacheBase2 = virt_addr { 0xFFC80000 };

   info->physDataAreaStart = processData->ramPartitionAllocation.dataStart;
   info->physDataAreaEnd = processData->ramPartitionAllocation.availStart;

   info->physAvailStart = processData->ramPartitionAllocation.availStart;
   info->physAvailEnd = processData->ramPartitionAllocation.codeGenStart;

   info->physCodeGenStart = processData->ramPartitionAllocation.codeGenStart;
   info->physCodeGenEnd = processData->ramPartitionAllocation.codeStart;

   info->titleId = processData->titleInfo.titleId;

   if (startInfo->coreinit) {
      auto coreinit = startInfo->coreinit;
      info->coreinit.loaderHandle = coreinit->moduleNameBuffer;
      info->coreinit.textAddr = coreinit->textAddr;
      info->coreinit.textOffset = coreinit->textOffset;
      info->coreinit.textSize = coreinit->textSize;
      info->coreinit.dataAddr = coreinit->dataAddr;
      info->coreinit.dataOffset = coreinit->dataOffset;
      info->coreinit.dataSize = coreinit->dataSize;
      info->coreinit.loadAddr = coreinit->loadAddr;
      info->coreinit.loadOffset = coreinit->loadOffset;
      info->coreinit.loadSize = coreinit->loadSize;
   }
}

void
getArgStr(virt_ptr<char> buffer,
          uint32_t size)
{
   auto processData = getCurrentProcessData();
   auto length = processData->argstr.size();
   if (length >= size) {
      length = size - 1;
   }

   std::memcpy(buffer.getRawPointer(),
               processData->argstr.data(),
               length);
   buffer[length] = char { 0 };
}

void
getInfo(InfoType type,
        virt_ptr<void> buffer,
        uint32_t size)
{
   switch (type) {
   case InfoType::Type0:
      getType0Info(virt_cast<Info0 *>(buffer), size);
      break;
   case InfoType::ArgStr:
      getArgStr(virt_cast<char *>(buffer), size);
      break;
   default:
      decaf_abort(fmt::format("Unexpected kernel info type {}",
                              static_cast<unsigned>(type)));
   }
}

} // namespace cafe::kernel
