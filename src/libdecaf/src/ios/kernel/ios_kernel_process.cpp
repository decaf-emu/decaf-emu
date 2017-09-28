#include "ios_kernel_process.h"
#include "ios_kernel_thread.h"

#include "kernel/kernel_memory.h"

#include <array>
#include <cstring>
#include <common/frameallocator.h>

namespace ios::kernel
{

static std::array<FrameAllocator, NumIosProcess>
sProcessStaticAllocators;

Error
IOS_GetCurrentProcessId()
{
   auto thread = internal::getCurrentThread();
   if (!thread) {
      return Error::Invalid;
   }

   return static_cast<Error>(thread->pid);
}

Error
IOS_GetProcessName(ProcessId process,
                   char *buffer)
{
   const char *name = nullptr;

   switch (process) {
   case ProcessId::KERNEL:
      name = "IOS-KERNEL";
      break;
   case ProcessId::MCP:
      name = "IOS-MCP";
      break;
   case ProcessId::BSP:
      name = "IOS-BSP";
      break;
   case ProcessId::CRYPTO:
      name = "IOS-CRYPTO";
      break;
   case ProcessId::USB:
      name = "IOS-USB";
      break;
   case ProcessId::FS:
      name = "IOS-FS";
      break;
   case ProcessId::PAD:
      name = "IOS-PAD";
      break;
   case ProcessId::NET:
      name = "IOS-NET";
      break;
   case ProcessId::ACP:
      name = "IOS-ACP";
      break;
   case ProcessId::NSEC:
      name = "IOS-NSEC";
      break;
   case ProcessId::AUXIL:
      name = "IOS-AUXIL";
      break;
   case ProcessId::NIM:
      name = "IOS-NIM";
      break;
   case ProcessId::FPD:
      name = "IOS-FPD";
      break;
   case ProcessId::IOSTEST:
      name = "IOS-TEST";
      break;
   case ProcessId::COSKERNEL:
      name = "COS-KERNEL";
      break;
   case ProcessId::COSROOT:
      name = "COS-ROOT";
      break;
   case ProcessId::COS02:
      name = "COS-02";
      break;
   case ProcessId::COS03:
      name = "COS-03";
      break;
   case ProcessId::COSOVERLAY:
      name = "COS-OVERLAY";
      break;
   case ProcessId::COSHBM:
      name = "COS-HBM";
      break;
   case ProcessId::COSERROR:
      name = "COS-ERROR";
      break;
   case ProcessId::COSMASTER:
      name = "COS-MASTER";
      break;
   default:
      return Error::Invalid;
   }

   strcpy(buffer, name);
   return Error::OK;
}

namespace internal
{

static void
initialiseAllocator(FrameAllocator &allocator, cpu::PhysicalAddressRange range)
{
   allocator = FrameAllocator {
      phys_ptr<uint8_t> { range.start }.getRawPointer(),
      range.size
   };
}

void
initialiseProcessStaticAllocators()
{
   using ::kernel::getPhysicalRange;
   using ::kernel::PhysicalRegion;

   /*
   ENUM_VALUE(BSP,                  2)
   ENUM_VALUE(CRYPTO,               3)
   ENUM_VALUE(USB,                  4)
   ENUM_VALUE(FS,                   5)
   ENUM_VALUE(PAD,                  6)
   ENUM_VALUE(NET,                  7)
   ENUM_VALUE(ACP,                  8)
   ENUM_VALUE(NSEC,                 9)
   ENUM_VALUE(AUXIL,                10)
   ENUM_VALUE(NIM,                  11)
   ENUM_VALUE(FPD,                  12)
   ENUM_VALUE(IOSTEST,              13)
   */
   initialiseAllocator(sProcessStaticAllocators[ProcessId::KERNEL],
                       getPhysicalRange(PhysicalRegion::MEM0IosKernel));

   initialiseAllocator(sProcessStaticAllocators[ProcessId::MCP],
                       getPhysicalRange(PhysicalRegion::MEM0IosMcp));
}

ProcessId
getCurrentProcessId()
{
   auto thread = getCurrentThread();
   if (!thread) {
      return ProcessId::KERNEL;
   }

   return thread->pid;
}

phys_ptr<void>
allocProcessStatic(size_t size)
{
   auto pid = getCurrentProcessId();
   decaf_check(pid < sProcessStaticAllocators.size());

   auto &allocator = sProcessStaticAllocators[pid];
   auto buffer = allocator.allocate(size);
   std::memset(buffer, 0, size);
   return cpu::translatePhysical(buffer);
}

} // namespace internal

} // namespace ios::kernel
