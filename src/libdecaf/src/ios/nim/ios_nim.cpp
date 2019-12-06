#include "ios_nim.h"
#include "ios_nim_log.h"
#include "ios_nim_boss_server.h"
#include "ios_nim_nim_server.h"

#include "decaf_log.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/nn/ios_nn.h"

using namespace ios::kernel;

namespace ios::nim
{

constexpr auto LocalHeapSize = 0x20000u;
constexpr auto CrossHeapSize = 0x24E400u;

static phys_ptr<void> sLocalHeapBuffer = nullptr;

namespace internal
{

Logger nimLog = { };

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
   internal::nimLog = decaf::makeLogger("IOS_NIM");

   // Initialise static memory
   internal::initialiseStaticData();
   internal::initialiseStaticBossServerData();
   internal::initialiseStaticNimServerData();

   // Initialise nn for current process
   nn::initialiseProcess();

   // Initialise process heaps
   error = IOS_CreateLocalProcessHeap(sLocalHeapBuffer, LocalHeapSize);
   if (error < Error::OK) {
      internal::nimLog->error(
         "processEntryPoint: IOS_CreateLocalProcessHeap failed with error = {}", error);
      return error;
   }

   error = IOS_CreateCrossProcessHeap(CrossHeapSize);
   if (error < Error::OK) {
      internal::nimLog->error(
         "processEntryPoint: IOS_CreateCrossProcessHeap failed with error = {}", error);
      return error;
   }

   // Start /dev/boss server
   error = internal::startBossServer();
   if (error < Error::OK) {
      internal::nimLog->error("Failed to start boss server");
      return error;
   }

   // Start /dev/nim server
   error = internal::startNimServer();
   if (error < Error::OK) {
      internal::nimLog->error("Failed to start nim server");
      return error;
   }

   internal::joinNimServer();
   internal::joinBossServer();

   return Error::OK;
}

} // namespace ios::nim
