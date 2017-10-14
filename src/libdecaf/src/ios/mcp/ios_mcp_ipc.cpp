#include "ios_mcp_ipc.h"
#include "ios_mcp_enum.h"

#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_ipc.h"
#include "ios/ios_stackobject.h"

namespace ios::mcp
{

constexpr auto DeviceNameLength = 32u;

using namespace kernel;

static phys_ptr<void>
allocIpcData(uint32_t size)
{
   auto buffer = IOS_HeapAlloc(CrossProcessHeapId, size);

   if (buffer) {
      std::memset(buffer.getRawPointer(), 0, size);
   }

   return buffer;
}

static void
freeIpcData(phys_ptr<void> data)
{
   IOS_HeapFree(CrossProcessHeapId, data);
}

Error
MCP_Open()
{
   return IOS_Open("/dev/mcp", OpenMode::None);
}

Error
MCP_Close(MCPHandle handle)
{
   return IOS_Close(handle);
}

Error
MCP_RegisterResourceManager(std::string_view device,
                            kernel::MessageQueueId queue)
{
   StackObject<uint32_t> resourceManagerId;
   auto error = IOS_Open("/dev/pm", OpenMode::None);
   if (error < Error::OK) {
      return error;
   }

   // Alloc a cross process buffer to copy the name to
   auto handle = static_cast<ResourceHandleId>(error);
   auto nameBuffer = phys_cast<char *>(allocIpcData(DeviceNameLength + 1));
   if (!nameBuffer) {
      error = Error::FailAlloc;
      goto out;
   }

   // Ensure device name does not exceed DeviceNameLength
   if (device.size() >= 32) {
      device.remove_suffix(device.size() - DeviceNameLength);
   }

   // Copy device to ipc buffer
   std::copy(device.begin(), device.end(), nameBuffer.getRawPointer());
   nameBuffer[device.size()] = char { 0 };

   // Send ioctl to get the manager id
   error = IOS_Ioctl(handle,
                     PMCommand::GetResourceManagerId,
                     nameBuffer, DeviceNameLength + 1,
                     nullptr, 0);

   if (error < Error::OK) {
      goto out;
   }

   *resourceManagerId = static_cast<uint32_t>(error);

   // Register resource manager with kernel
   error = IOS_RegisterResourceManager(device, queue);

   if (error < Error::OK) {
      goto out;
   }

   // Register resource manager with MCP
   error = IOS_Ioctl(handle,
                     PMCommand::RegisterResourceManager,
                     resourceManagerId, sizeof(uint32_t),
                     nullptr, 0);

out:
   if (nameBuffer) {
      freeIpcData(nameBuffer);
   }

   IOS_Close(handle);
   return error;
}

} // namespace ios::mcp
