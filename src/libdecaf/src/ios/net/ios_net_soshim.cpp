#include "ios_net_soshim.h"
#include "ios_net_socket_request.h"

#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_ipc.h"
#include "ios/ios_stackobject.h"

namespace ios::net
{

using namespace kernel;

static phys_ptr<void>
allocIpcData(uint32_t size)
{
   auto buffer = IOS_HeapAlloc(CrossProcessHeapId, size);

   if (buffer) {
      std::memset(buffer.get(), 0, size);
   }

   return buffer;
}

static void
freeIpcData(phys_ptr<void> data)
{
   IOS_HeapFree(CrossProcessHeapId, data);
}

Error
SOShim_Open()
{
   return IOS_Open("/dev/socket", OpenMode::None);
}

Error
SOShim_Close(SOShimHandle handle)
{
   return IOS_Close(handle);
}

Error
SOShim_GetProcessSocketHandle(SOShimHandle handle,
                           TitleId titleId,
                           ProcessId processId)
{
   auto request = phys_cast<SocketGetProcessSocketHandle *>(allocIpcData(sizeof(SocketGetProcessSocketHandle)));
   if (!request) {
      return Error::Access;
   }

   request->titleId = titleId;
   request->processId = processId;

   auto error = IOS_Ioctl(handle,
                          SocketCommand::GetProcessSocketHandle,
                          request,
                          sizeof(SocketGetProcessSocketHandle),
                          nullptr, 0);

   freeIpcData(request);
   return static_cast<Error>(error);
}

} // namespace ios::net
