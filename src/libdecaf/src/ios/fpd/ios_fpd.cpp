#include "ios_fpd.h"
#include "ios_fpd_log.h"
#include "ios_fpd_act_server.h"
#include "ios_fpd_act_accountdata.h"

#include "decaf_log.h"
#include "ios/kernel/ios_kernel_heap.h"

namespace ios::fpd
{

using namespace ios::kernel;

constexpr auto LocalHeapSize = 0xA0000u;
constexpr auto CrossHeapSize = 0xC0000u;

static phys_ptr<void> sLocalHeapBuffer = nullptr;

namespace internal
{

Logger fpdLog = { };

void
initialiseStaticData()
{
   sLocalHeapBuffer = allocProcessLocalHeap(LocalHeapSize);
}

} // namespace internal

Error
processEntryPoint(phys_ptr<void> /* context */)
{
   auto error = Error::OK;

   // Initialise logger
   internal::fpdLog = decaf::makeLogger("IOS_FPD");

   internal::initialiseStaticData();
   internal::initialiseStaticActServerData();
   internal::initialiseStaticAccountData();

   // Initialise process heaps
   error = IOS_CreateLocalProcessHeap(sLocalHeapBuffer, LocalHeapSize);
   if (error < Error::OK) {
      internal::fpdLog->error(
         "processEntryPoint: IOS_CreateLocalProcessHeap failed with error = {}", error);
      return error;
   }

   error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      internal::fpdLog->error(
         "processEntryPoint: IOS_CreateCrossProcessHeap failed with error = {}", error);
      return error;
   }

   error = internal::startActServer();
   if (error < Error::OK) {
      internal::fpdLog->error(
         "processEntryPoint: startActServer failed with error = {}", error);
      return error;
   }

   // TODO: Start /dev/fpd thread, Join /dev/fpd thread

   return error;
}

} // namespace ios::fpd
