#pragma once
#include "cafe_kernel_process.h"

#include <libcpu/be2_struct.h>

namespace cafe::kernel
{

enum class InfoType
{
   Type0 = 0,
   Type6 = 6,
   ArgStr = 3,
};

struct Info0
{
   struct CoreinitInfo
   {
      be2_virt_ptr<void> loaderHandle;
      be2_val<virt_addr> textAddr;
      be2_val<uint32_t> textOffset;
      be2_val<uint32_t> textSize;
      be2_val<virt_addr> dataAddr;
      be2_val<uint32_t> dataOffset;
      be2_val<uint32_t> dataSize;
      be2_val<virt_addr> loadAddr;
      be2_val<uint32_t> loadOffset;
      be2_val<uint32_t> loadSize;
   };

   be2_val<UniqueProcessId> upid;
   be2_val<RamPartitionId> rampid;
   be2_val<ProcessFlags> appFlags;
   be2_val<virt_addr> dataAreaStart;
   be2_val<virt_addr> dataAreaEnd;
   be2_val<phys_addr> physDataAreaStart;
   be2_val<phys_addr> physDataAreaEnd;
   be2_val<phys_addr> physAvailStart;
   be2_val<phys_addr> physAvailEnd;
   be2_val<phys_addr> physCodeGenStart;
   be2_val<phys_addr> physCodeGenEnd;
   be2_val<virt_addr> sdaBase;
   be2_val<virt_addr> sda2Base;
   be2_val<uint32_t> systemHeapSize;
   be2_val<virt_addr> stackEnd0;
   be2_val<virt_addr> stackEnd1;
   be2_val<virt_addr> stackEnd2;
   be2_val<virt_addr> stackBase0;
   be2_val<virt_addr> stackBase1;
   be2_val<virt_addr> stackBase2;
   be2_val<virt_addr> exceptionStackEnd0;
   be2_val<virt_addr> exceptionStackEnd1;
   be2_val<virt_addr> exceptionStackEnd2;
   be2_val<virt_addr> exceptionStackBase0;
   be2_val<virt_addr> exceptionStackBase1;
   be2_val<virt_addr> exceptionStackBase2;
   be2_val<virt_addr> lockedCacheBase0;
   be2_val<virt_addr> lockedCacheBase1;
   be2_val<virt_addr> lockedCacheBase2;
   be2_struct<CoreinitInfo> coreinit;
   be2_val<uint32_t> unk0x9C;
   be2_val<TitleId> titleId;
};
CHECK_OFFSET(Info0, 0x00, upid);
CHECK_OFFSET(Info0, 0x04, rampid);
CHECK_OFFSET(Info0, 0x08, appFlags);
CHECK_OFFSET(Info0, 0x0C, dataAreaStart);
CHECK_OFFSET(Info0, 0x10, dataAreaEnd);
CHECK_OFFSET(Info0, 0x14, physDataAreaStart);
CHECK_OFFSET(Info0, 0x18, physDataAreaEnd);
CHECK_OFFSET(Info0, 0x1C, physAvailStart);
CHECK_OFFSET(Info0, 0x20, physAvailEnd);
CHECK_OFFSET(Info0, 0x24, physCodeGenStart);
CHECK_OFFSET(Info0, 0x28, physCodeGenEnd);
CHECK_OFFSET(Info0, 0x2C, sdaBase);
CHECK_OFFSET(Info0, 0x30, sda2Base);
CHECK_OFFSET(Info0, 0x34, systemHeapSize);
CHECK_OFFSET(Info0, 0x38, stackEnd0);
CHECK_OFFSET(Info0, 0x3C, stackEnd1);
CHECK_OFFSET(Info0, 0x40, stackEnd2);
CHECK_OFFSET(Info0, 0x44, stackBase0);
CHECK_OFFSET(Info0, 0x48, stackBase1);
CHECK_OFFSET(Info0, 0x4C, stackBase2);
CHECK_OFFSET(Info0, 0x50, exceptionStackEnd0);
CHECK_OFFSET(Info0, 0x54, exceptionStackEnd1);
CHECK_OFFSET(Info0, 0x58, exceptionStackEnd2);
CHECK_OFFSET(Info0, 0x5C, exceptionStackBase0);
CHECK_OFFSET(Info0, 0x60, exceptionStackBase1);
CHECK_OFFSET(Info0, 0x64, exceptionStackBase2);
CHECK_OFFSET(Info0, 0x68, lockedCacheBase0);
CHECK_OFFSET(Info0, 0x6C, lockedCacheBase1);
CHECK_OFFSET(Info0, 0x70, lockedCacheBase2);
CHECK_OFFSET(Info0, 0x74, coreinit);
CHECK_OFFSET(Info0, 0x9C, unk0x9C);
CHECK_OFFSET(Info0, 0xA0, titleId);
CHECK_SIZE(Info0, 0xA8);

struct Info6
{
   be2_val<uint64_t> osTitleId;
   be2_val<uint32_t> unk0x08;
   PADDING(0x108 - 0xC);
};
CHECK_OFFSET(Info6, 0x00, osTitleId);
CHECK_OFFSET(Info6, 0x08, unk0x08);
CHECK_SIZE(Info6, 0x108);

void
getType0Info(virt_ptr<Info0> info,
             uint32_t size);

void
getType6Info(virt_ptr<Info6> info,
             uint32_t size);

void
getArgStr(virt_ptr<char> buffer,
          uint32_t size);

void
getInfo(InfoType type,
        virt_ptr<void> buffer,
        uint32_t size);

} // namespace cafe::kernel
