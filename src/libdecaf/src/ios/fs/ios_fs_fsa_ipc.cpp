#include "ios_fs_fsa_ipc.h"
#include "ios_fs_fsa_request.h"
#include "ios_fs_fsa_response.h"
#include "ios_fs_fsa_types.h"
#include "ios/ios_stackobject.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_ipc.h"

namespace ios::fs
{

struct FSAIpcData
{
   be2_struct<FSARequest> request;
   be2_struct<FSAResponse> response;
};

static FSAStatus
allocFsaIpcData(phys_ptr<FSAIpcData> *outIpcData)
{
   auto buffer = kernel::IOS_HeapAlloc(kernel::CrossProcessHeapId,
                                       sizeof(FSAIpcData));

   if (!buffer) {
      return FSAStatus::OutOfResources;
   }

   std::memset(buffer.getRawPointer(), 0, sizeof(FSAIpcData));
   *outIpcData = phys_cast<FSAIpcData>(buffer);
   return FSAStatus::OK;
}

static void
freeFsaIpcData(phys_ptr<FSAIpcData> ipcData)
{
   kernel::IOS_HeapFree(kernel::CrossProcessHeapId, ipcData);
}

FSAStatus
FSAOpenFile(kernel::ResourceHandleId resourceHandleId,
            std::string_view name,
            std::string_view mode,
            FSAFileHandle *outHandle)
{
   phys_ptr<FSAIpcData> ipcData;

   if (name.size() == 0) {
      return FSAStatus::InvalidPath;
   }

   if (mode.size() == 0) {
      return FSAStatus::InvalidParam;
   }

   if (!outHandle) {
      return FSAStatus::InvalidBuffer;
   }

   auto status = allocFsaIpcData(&ipcData);
   if (status < FSAStatus::OK) {
      return status;
   }

   auto request = phys_addrof(ipcData->request);
   auto response = phys_addrof(ipcData->response);

   std::strncpy(phys_addrof(request->openFile.path).getRawPointer(),
                name.data(),
                request->openFile.path.size());

   std::strncpy(phys_addrof(request->openFile.mode).getRawPointer(),
                mode.data(),
                request->openFile.mode.size());

   // Maybe this is like permission 600? 0x06 0x00 0x00
   request->openFile.unk0x290 = 0x60000u;

   auto error = kernel::IOS_Ioctl(resourceHandleId,
                                  FSACommand::OpenFile,
                                  request,
                                  sizeof(FSARequest),
                                  response,
                                  sizeof(FSAResponse));

   *outHandle = response->openFile.handle;
   freeFsaIpcData(ipcData);
   return static_cast<FSAStatus>(error);
}

} // namespace ios::fs
