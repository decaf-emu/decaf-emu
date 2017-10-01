#include "ios_mcp.h"
#include "ios_mcp_pm_thread.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"

#include "ios/ios_stackobject.h"

#include <common/log.h>

namespace ios::mcp
{

using namespace ios::kernel;

constexpr auto LocalHeapSize = 0x8000u;
constexpr auto CrossHeapSize = 0x31E000u;

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

static void
initialiseClientCaps()
{
   StackObject<uint64_t> mask;

   struct
   {
      ProcessId pid;
      FeatureId fid;
      uint64_t mask;
   } caps[] = {
      { ProcessId::MCP,    0x7FFFFFFF, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::CRYPTO,          1,               0xFF },
      { ProcessId::USB,             1,                0xF },
      { ProcessId::USB,          0xC, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::USB,          9, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::USB,        0xB,          0x3300300 },
      { ProcessId::FS,          1,                0xF },
      { ProcessId::FS,          3,                  3 },
      { ProcessId::FS,          9,                  1 },
      { ProcessId::FS,        0xB, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::FS,          2, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::FS,        0xC,                  1 },
      { ProcessId::PAD,        0xB,           0x101000 },
      { ProcessId::PAD,          1,                0xF },
      { ProcessId::PAD,        0xD,                  1 },
      { ProcessId::PAD,       0x18, 0xFFFFFFFFFFFFFFFF },
      { ProcessId::NET,          1,                0xF },
      { ProcessId::NET,          8,                  1 },
      { ProcessId::NET,        0xC,                  1 },
      { ProcessId::NET,       0x1A, 0xFFFFFFFFFFFFFFFF },
   };

   for (auto &cap : caps) {
      *mask = cap.mask;
      IOS_SetClientCapabilities(cap.pid, cap.fid, mask);
   }
}

} // namespace internal


Error
processEntryPoint(phys_ptr<void> /* context */)
{
   // Initialise process static data
   internal::initialiseStaticData();
   internal::initialiseStaticPmThreadData();

   // Initialise process heaps
   auto error = IOS_CreateLocalProcessHeap(phys_addrof(sData->localHeapBuffer),
                                           static_cast<uint32_t>(sData->localHeapBuffer.size()));
   if (error < Error::OK) {
      gLog->error("Failed to create local process heap, error = {}.", error);
      return error;
   }

   error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      gLog->error("Failed to create cross process heap, error = {}.", error);
      return error;
   }

   // TODO: Lots of unknown stuff

   // Start pm thread
   error = internal::startPmThread();
   if (error < Error::OK) {
      gLog->error("Failed to start pm thread, error = {}.", error);
      return error;
   }

   // Set process client caps
   internal::initialiseClientCaps();

   // TODO: startMcpThread

   // TODO: startPpcKernelThread!!

   // Handle all pending resource manager registrations
   error = internal::handleResourceManagerRegistrations();
   if (error < Error::OK) {
      gLog->error("Failed to handle resource manager registrations, error = {}.", error);
      return error;
   }

   // Main thread is a IOS_HandleEvent DeviceId 9 message loop
   return Error::OK;
}

} // namespace ios::mcp
