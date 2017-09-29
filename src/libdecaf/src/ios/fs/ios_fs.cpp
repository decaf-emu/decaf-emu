#include "ios_fs.h"
#include "ios_fs_fsa_thread.h"

#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/kernel/ios_kernel_thread.h"

#include <common/log.h>

namespace ios::fs
{

constexpr auto LocalHeapSize = 0x56000u;
constexpr auto CrossHeapSize = 0x1D80000u;

struct StaticData
{
   be2_array<std::byte, LocalHeapSize> localHeapBuffer;
};

static phys_ptr<StaticData>
sData = nullptr;

namespace internal
{

static void
initialiseStaticData()
{
   sData = kernel::allocProcessStatic<StaticData>();
}

} // namespace internal

static Error
fsServiceThreadEntry(phys_ptr<void> /* context */)
{
   // The list of devices the service thread is responsible for.
   static const char *sDevices[] = {
      "/dev/df",
      "/dev/atfs",
      "/dev/isfs",
      "/dev/wfs",
      "/dev/pcfs",
      "/dev/rbfs",
      "/dev/fat",
      "/dev/fla",
      "/dev/ums",
      "/dev/ahcimgr",
      "/dev/shdd",
      "/dev/md",
      "/dev/scfm",
      "/dev/mmc",
      "/dev/timetrace",
      "/dev/tcp_pcfs",
   };

   return Error::OK;
}

Error
processEntryPoint(phys_ptr<void> context)
{
   // Initialise static memory
   internal::initialiseStaticData();
   internal::initialiseStaticFsaThreadData();

   // Initialise process heaps
   auto error = kernel::IOS_CreateLocalProcessHeap(phys_addrof(sData->localHeapBuffer),
                                                   static_cast<uint32_t>(sData->localHeapBuffer.size()));
   if (error < Error::OK) {
      gLog->error("Failed to create local process heap, error = {}.", error);
      return error;
   }

   error = kernel::IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      gLog->error("Failed to create cross process heap, error = {}.", error);
      return error;
   }

   // TODO: Get clock info from bsp

   // TODO: Start dk thread

   // TODO: Start odm thread

   // Start FSA thread.
   error = internal::startFsaThread();
   if (error < Error::OK) {
      gLog->error("Failed to start FSA thread");
      return error;
   }

   // TODO: Start service thread.

   // TODO: Start ramdisk service thread.

   // Suspend current thread.
   error = kernel::IOS_GetCurrentThreadId();
   if (error < Error::OK) {
      gLog->error("Failed to get current thread id");
      return error;
   }

   auto threadId = static_cast<kernel::ThreadId>(error);
   error = kernel::IOS_SuspendThread(threadId);
   if (error < Error::OK) {
      gLog->error("Failed to suspend root fs thread");
      return error;
   }

   return Error::OK;
}

} // namespace ios