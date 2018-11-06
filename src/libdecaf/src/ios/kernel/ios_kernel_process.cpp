#include "ios_kernel_process.h"
#include "ios_kernel_thread.h"

#include <array>
#include <cstring>
#include <common/frameallocator.h>
#include <common/strutils.h>

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


phys_ptr<char>
allocProcessStatic(std::string_view str)
{
   auto buffer = phys_cast<char *>(allocProcessStatic(str.size() + 1, 4));
   std::copy(str.begin(), str.end(), buffer.get());
   buffer[str.size()] = char { 0 };
   return buffer;
}


phys_ptr<void>
allocProcessStatic(ProcessId pid,
                   size_t size,
                   size_t align)
{
   decaf_check(pid < sProcessStaticAllocators.size());
   auto &allocator = sProcessStaticAllocators[pid];
   auto buffer = allocator.allocate(size, align);
   std::memset(buffer, 0, size);
   return phys_cast<void *>(cpu::translatePhysical(buffer));
}


phys_ptr<void>
allocProcessStatic(size_t size,
                   size_t align)
{
   return allocProcessStatic(internal::getCurrentProcessId(), size, align);
}


phys_ptr<void>
allocProcessLocalHeap(size_t size)
{
   auto pid = internal::getCurrentProcessId();
   auto &allocator = sProcessStaticAllocators[pid];
   auto buffer = allocator.allocate(size, 0x20);
   std::memset(buffer, 0, size);
   return phys_cast<void *>(cpu::translatePhysical(buffer));
}


namespace internal
{

void
initialiseProcessStaticAllocators()
{
   struct PhysicalMemoryLayout
   {
      ProcessId id;
      phys_addr addr;
      uint32_t size;
   };

   static constexpr PhysicalMemoryLayout
   ProcessMemoryLayout[] = {
      { ProcessId::KERNEL,    phys_addr { 0x08120000 },   0xA0000 },
      { ProcessId::MCP,       phys_addr { 0x081C0000 },   0xC0000 },
      { ProcessId::BSP,       phys_addr { 0x13CC0000 },   0xC0000 },
      { ProcessId::CRYPTO,    phys_addr { 0x08280000 },   0x40000 },
      { ProcessId::USB,       phys_addr { 0x10100000 },  0x600000 },
      { ProcessId::FS,        phys_addr { 0x10700000 }, 0x1800000 },
      { ProcessId::PAD,       phys_addr { 0x11F00000 },  0x400000 },
      { ProcessId::NET,       phys_addr { 0x12300000 },  0x600000 },
      { ProcessId::ACP,       phys_addr { 0x12900000 },  0x2C0000 },
      { ProcessId::NSEC,      phys_addr { 0x12BC0000 },  0x300000 },
      { ProcessId::AUXIL,     phys_addr { 0x13C00000 },   0xC0000 },
      { ProcessId::NIM,       phys_addr { 0x12EC0000 },  0x780000 },
      { ProcessId::FPD,       phys_addr { 0x13640000 },  0x400000 },
      { ProcessId::TEST,      phys_addr { 0x13A40000 },  0x1C0000 },
   };

   for (auto &layout : ProcessMemoryLayout) {
      sProcessStaticAllocators[layout.id] =
         FrameAllocator {
            phys_cast<uint8_t *>(layout.addr).get(),
            layout.size
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
