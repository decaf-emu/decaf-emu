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
   case ProcessId::TEST:
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


phys_ptr<void>
allocProcessStatic(size_t size)
{
   return allocProcessStatic(internal::getCurrentProcessId(), size);
}


phys_ptr<char>
allocProcessStatic(std::string_view str)
{
   auto buffer = phys_cast<char>(allocProcessStatic(str.size() + 1));
   std::copy(str.begin(), str.end(), buffer.getRawPointer());
   buffer[str.size()] = char { 0 };
   return buffer;
}


phys_ptr<void>
allocProcessStatic(ProcessId pid,
                   size_t size)
{
   decaf_check(pid < sProcessStaticAllocators.size());
   auto &allocator = sProcessStaticAllocators[pid];
   auto buffer = allocator.allocate(size, 16);
   std::memset(buffer, 0, size);
   return cpu::translatePhysical(buffer);
}


phys_ptr<void>
allocProcessLocalHeap(size_t size)
{
   auto pid = internal::getCurrentProcessId();
   auto &allocator = sProcessStaticAllocators[pid];
   auto buffer = allocator.allocate(size, 0x20);
   std::memset(buffer, 0, size);
   return cpu::translatePhysical(buffer);
}


namespace internal
{

void
initialiseProcessStaticAllocators()
{
   using ::kernel::getPhysicalRange;
   using ::kernel::PhysicalRegion;
   static constexpr std::pair<ProcessId, PhysicalRegion>
   processRegionMap[] = {
      { ProcessId::KERNEL,    PhysicalRegion::MEM0IosKernel },
      { ProcessId::MCP,       PhysicalRegion::MEM0IosMcp },
      { ProcessId::BSP,       PhysicalRegion::MEM2IosBsp },
      { ProcessId::CRYPTO,    PhysicalRegion::MEM0IosCrypto },
      { ProcessId::USB,       PhysicalRegion::MEM2IosUsb },
      { ProcessId::FS,        PhysicalRegion::MEM2IosFs },
      { ProcessId::PAD,       PhysicalRegion::MEM2IosPad },
      { ProcessId::NET,       PhysicalRegion::MEM2IosNet },
      { ProcessId::ACP,       PhysicalRegion::MEM2IosAcp },
      { ProcessId::NSEC,      PhysicalRegion::MEM2IosNsec },
      { ProcessId::AUXIL,     PhysicalRegion::MEM2IosAuxil },
      { ProcessId::NIM,       PhysicalRegion::MEM2IosNim },
      { ProcessId::FPD,       PhysicalRegion::MEM2IosFpd },
      { ProcessId::TEST,      PhysicalRegion::MEM2IosTest },
   };

   for (auto &processMap : processRegionMap) {
      auto range = getPhysicalRange(processMap.second);
      sProcessStaticAllocators[processMap.first] = FrameAllocator {
            phys_ptr<uint8_t> { range.start }.getRawPointer(),
            range.size
         };
   }
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

} // namespace internal

} // namespace ios::kernel
