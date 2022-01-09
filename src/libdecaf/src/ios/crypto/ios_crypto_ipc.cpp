#include "ios_crypto_ipc.h"
#include "ios_crypto_request.h"

#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_ipc.h"
#include "ios/ios_stackobject.h"

namespace ios::crypto
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
IOSC_Open()
{
   return IOS_Open("/dev/crypto", OpenMode::None);
}

Error
IOSC_Close(IOSCHandle handle)
{
   return IOS_Close(handle);
}

IOSCError
IOSC_Decrypt(IOSCHandle handle,
             IOSCKeyHandle keyHandle,
             phys_ptr<const void> ivData, uint32_t ivSize,
             phys_ptr<const void> inputData, uint32_t inputSize,
             phys_ptr<void> outputData, uint32_t outputSize)
{
   if (!align_check(inputData.get(), 0x10u) || !align_check(outputData.get(), 0x10u)) {
      return IOSCError::InvalidParam;
   }

   auto request = phys_cast<IOSCRequestDecrypt *>(
      allocIpcData(sizeof(IOSCRequestDecrypt)));
   request->unknown0x00 = 0u;
   request->unknown0x04 = 1u;
   request->keyHandle = keyHandle;

   auto vecs = phys_cast<IoctlVec *>(allocIpcData(sizeof(IoctlVec) * 4));
   vecs[0].paddr = phys_cast<phys_addr>(request);
   vecs[0].len = static_cast<uint32_t>(sizeof(IOSCRequestDecrypt));

   vecs[1].paddr = phys_cast<phys_addr>(ivData);
   vecs[1].len = ivSize;

   vecs[2].paddr = phys_cast<phys_addr>(inputData);
   vecs[2].len = inputSize;

   vecs[3].paddr = phys_cast<phys_addr>(outputData);
   vecs[3].len = outputSize;

   auto error = IOS_Ioctlv(handle, IOSCCommand::Decrypt, 3u, 1u, vecs);

   freeIpcData(request);
   freeIpcData(vecs);
   return static_cast<IOSCError>(error);
}

} // namespace ios::crypto
