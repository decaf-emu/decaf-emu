#include "ios_acp_enum.h"
#include "ios_acp_nnsm_ipc.h"

#include "ios/ios_enum.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_ipc.h"

#include <string_view>

using namespace ios::kernel;

namespace ios::acp
{

Error
NNSM_RegisterServer(std::string_view device)
{
   // Open nnsm
   auto error = IOS_Open("/dev/nnsm", OpenMode::None);
   if (error < Error::OK) {
      return error;
   }
   auto handle = static_cast<ResourceHandleId>(error);

   // Allocate buffer for name
   auto nameBufferLen = static_cast<uint32_t>(device.size() + 1);
   auto nameBuffer = IOS_HeapAllocAligned(CrossProcessHeapId, nameBufferLen, 4);
   std::memcpy(nameBuffer.get(), device.data(), nameBufferLen - 1);
   phys_cast<char *>(nameBuffer)[nameBufferLen - 1] = char { 0 };

   // Send ioctl
   error = IOS_Ioctl(handle,
                     NssmCommand::RegisterService,
                     nullptr, 0,
                     nameBuffer, nameBufferLen);

   // Cleanup
   IOS_Close(handle);
   IOS_HeapFree(CrossProcessHeapId, nameBuffer);
   return error;
}

Error
NNSM_UnregisterServer(std::string_view device)
{
   // Open nnsm
   auto error = IOS_Open("/dev/nnsm", OpenMode::None);
   if (error < Error::OK) {
      return error;
   }
   auto handle = static_cast<ResourceHandleId>(error);

   // Allocate buffer for name
   auto nameBufferLen = static_cast<uint32_t>(device.size() + 1);
   auto nameBuffer = IOS_HeapAllocAligned(CrossProcessHeapId, nameBufferLen, 4);
   std::memcpy(nameBuffer.get(), device.data(), nameBufferLen - 1);
   phys_cast<char *>(nameBuffer)[nameBufferLen - 1] = char { 0 };

   // Send ioctl
   error = IOS_Ioctl(handle,
                     NssmCommand::UnregisterService,
                     nullptr, 0,
                     nameBuffer, nameBufferLen);

   // Cleanup
   IOS_Close(handle);
   IOS_HeapFree(CrossProcessHeapId, nameBuffer);
   return error;
}

} // namespace ios::acp
