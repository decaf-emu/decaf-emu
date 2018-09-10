#pragma once
#include "cafe/kernel/cafe_kernel_mmu.h"
#include "cafe/kernel/cafe_kernel_processid.h"
#include "cafe/loader/cafe_loader_init.h"
#include "ios/mcp/ios_mcp_mcp_types.h"

#include <array>
#include <libcpu/be2_struct.h>
#include <string_view>

namespace cafe::loader
{
struct RPL_STARTINFO;
};

namespace cafe::kernel
{

void
exitProcess(int code);

int
getProcessExitCode(RamPartitionId rampid);

namespace internal
{

struct ProcessPerCoreStartInfo
{
   UNKNOWN(0x8);
   be2_val<virt_addr> stackBase;
   be2_val<virt_addr> stackEnd;
   be2_val<uint32_t> sda2Base;
   be2_val<uint32_t> sdaBase;
   UNKNOWN(0x4);
   be2_val<uint32_t> unk0x1C;
   be2_val<virt_addr> entryPoint;
   be2_val<virt_addr> exceptionStackBase;
   be2_val<virt_addr> exceptionStackEnd;
};
CHECK_OFFSET(ProcessPerCoreStartInfo, 0x08, stackBase);
CHECK_OFFSET(ProcessPerCoreStartInfo, 0x0C, stackEnd);
CHECK_OFFSET(ProcessPerCoreStartInfo, 0x10, sda2Base);
CHECK_OFFSET(ProcessPerCoreStartInfo, 0x14, sdaBase);
CHECK_OFFSET(ProcessPerCoreStartInfo, 0x1C, unk0x1C);
CHECK_OFFSET(ProcessPerCoreStartInfo, 0x20, entryPoint);
CHECK_OFFSET(ProcessPerCoreStartInfo, 0x24, exceptionStackBase);
CHECK_OFFSET(ProcessPerCoreStartInfo, 0x28, exceptionStackEnd);
CHECK_SIZE(ProcessPerCoreStartInfo, 0x2C);

struct RamPartitionAllocation
{
   phys_addr dataStart;
   phys_addr availStart;
   phys_addr codeGenStart;
   phys_addr codeStart;
   phys_addr codeEnd;
   uint32_t codegen_core;
   uint32_t unk0x18;
   uint32_t unk0x1C;
};

struct RamPartitionData
{
   UniqueProcessId uniqueProcessId;
   RamPartitionId ramPartitionId;
   ios::mcp::MCPTitleId titleId;
   std::array<KernelProcessId, 3> coreKernelProcessId;
   loader::RPL_STARTINFO startInfo;
   internal::AddressSpace addressSpace;
   RamPartitionAllocation ramPartitionAllocation;
   std::string argstr;
   virt_ptr<loader::LOADED_RPL> loadedRpx;
   virt_ptr<loader::LOADED_RPL> loadedModuleList;
   ios::mcp::MCPPPrepareTitleInfo titleInfo;
   bool overlayArenaEnabled;
   std::array<ProcessPerCoreStartInfo, 3> perCoreStartInfo;
   int exitCode;
};
// CHECK_OFFSET(RamPartitionData, 0x04, rampId);
// CHECK_OFFSET(RamPartitionData, 0x08, titleId);
// CHECK_OFFSET(RamPartitionData, 0x10, sdkVersion);
// CHECK_OFFSET(RamPartitionData, 0x14, titleVersion);
// CHECK_OFFSET(RamPartitionData, 0x18, state);
// CHECK_OFFSET(RamPartitionData, 0x1C, perCoreUniqueProcessId);
// CHECK_OFFSET(RamPartitionData, 0x28, startInfo);
// CHECK_OFFSET(RamPartitionData, 0x50, addressSpace);
// CHECK_OFFSET(RamPartitionData, 0xE58, cmdFlags);
//
// CHECK_OFFSET(RamPartitionData, 0xE80, ramPartitionAllocation);
// CHECK_OFFSET(RamPartitionData, 0xEA0, numCodeAreaHeapBlocks);
// CHECK_OFFSET(RamPartitionData, 0xEA4, argstr);
// CHECK_OFFSET(RamPartitionData, 0xEA8, loadedRpx);
// CHECK_OFFSET(RamPartitionData, 0xEAC, loadedModuleList);
//
// CHECK_OFFSET(RamPartitionData, 0xECC, titleInfo);
//
// CHECK_OFFSET(RamPartitionData, 0x1118, userExceptionHandlersCore0);
// CHECK_OFFSET(RamPartitionData, 0x11CC, userExceptionHandlersCore1);
// CHECK_OFFSET(RamPartitionData, 0x1180, userExceptionHandlersCore2);
//
// CHECK_OFFSET(RamPartitionData, 0x1344, titleLoc);
// CHECK_OFFSET(RamPartitionData, 0x1348, overlay_arena);
//
// CHECK_OFFSET(RamPartitionData, 0x1698, perCoreStartInfo);
// CHECK_SIZE(RamPartitionData, 0x17a0);

RamPartitionData *
getCurrentRamPartitionData();

RamPartitionId
getCurrentRamPartitionId();

KernelProcessId
getCurrentKernelProcessId();

UniqueProcessId
getCurrentUniqueProcessId();

ios::mcp::MCPTitleId
getCurrentTitleId();

loader::RPL_STARTINFO *
getCurrentRamPartitionStartInfo();

RamPartitionData *
getRamPartitionData(RamPartitionId id);

void
setCoreToProcessId(RamPartitionId ramPartitionId,
                   KernelProcessId kernelProcessId);

void
loadGameProcess(std::string_view rpx,
                ios::mcp::MCPPPrepareTitleInfo &titleInfo);

void
finishInitAndPreload();

void
initialiseCoreProcess(int coreId,
                      RamPartitionId rampid,
                      UniqueProcessId upid,
                      KernelProcessId pid);

void
initialiseProcessData();

} // namespace internal

} // namespace cafe::kernel
