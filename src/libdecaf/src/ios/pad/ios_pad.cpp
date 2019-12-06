#include "ios_pad.h"
#include "ios_pad_btrm_device.h"
#include "ios_pad_log.h"

#include "decaf_log.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_messagequeue.h"

using namespace ios::kernel;

namespace ios::pad
{

constexpr auto LocalHeapSize = 0x100000u;
constexpr auto CrossHeapSize = 0x20000u;

static phys_ptr<void> sLocalHeapBuffer = nullptr;

namespace internal
{

Logger padLog = { };

static void
initialiseStaticData()
{
   sLocalHeapBuffer = kernel::allocProcessLocalHeap(LocalHeapSize);
}

} // namespace internal

Error
processEntryPoint(phys_ptr<void> /* context */)
{
   // Initialise logger
   internal::padLog = decaf::makeLogger("IOS_PAD");

   // Initialise static memory
   internal::initialiseStaticData();
   internal::initialiseStaticBtrmData();

   // Initialise process heaps
   auto error = kernel::IOS_CreateLocalProcessHeap(sLocalHeapBuffer, LocalHeapSize);
   if (error < Error::OK) {
      internal::padLog->error("Failed to create local process heap, error = {}.", error);
      return error;
   }

   error = kernel::IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      internal::padLog->error("Failed to create cross process heap, error = {}.", error);
      return error;
   }

   // TODO: Create /dev/ccr_io thread

   error = internal::startBtrmDeviceThread();
   if (error < Error::OK) {
      internal::padLog->error("Failed to start btrm device thread, error = {}.", error);
      return error;
   }

   return Error::OK;
}

} // namespace ios::pad
