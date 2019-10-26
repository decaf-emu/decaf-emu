#include "nsysnet.h"
#include "nsysnet_nssl.h"
#include "cafe/libraries/cafe_hle_stub.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_ios.h"
#include "cafe/libraries/coreinit/coreinit_ipcbufpool.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "ios/nsec/ios_nsec_nssl.h"

namespace cafe::nsysnet
{

using namespace coreinit;

using ios::nsec::NSSLCommand;
using ios::nsec::NSSLCreateContextRequest;

struct SslData
{
   static constexpr uint32_t MessageCount = 0x40;
   static constexpr uint32_t MessageSize = 0x100;

   be2_struct<OSMutex> lock;
   be2_val<IOSHandle> handle;
   be2_virt_ptr<IPCBufPool> messagePool;
   be2_array<uint8_t, MessageCount * MessageSize> messageBuffer;
   be2_val<uint32_t> messageCount;
};

static virt_ptr<SslData>
sSslData;


namespace internal
{

static bool isInitialised();
static virt_ptr<void> allocateIpcBuffer(uint32_t size);
static void freeIpcBuffer(virt_ptr<void> buf);

} // namespace internal

NSSLError
NSSLInit()
{
   OSLockMutex(virt_addrof(sSslData->lock));

   if (sSslData->handle < 0) {
      auto iosError = IOS_Open(make_stack_string("/dev/nsec/nssl"),
                               IOSOpenMode::None);

      if (iosError < 0) {
         OSUnlockMutex(virt_addrof(sSslData->lock));
         return NSSLError::IpcError;
      }

      sSslData->handle = static_cast<IOSHandle>(iosError);
   }

   if (!sSslData->messagePool) {
      sSslData->messagePool =
         IPCBufPoolCreate(virt_addrof(sSslData->messageBuffer),
                          static_cast<uint32_t>(sSslData->messageBuffer.size()),
                          SslData::MessageSize,
                          virt_addrof(sSslData->messageCount),
                          1);

      if (!sSslData->messagePool) {
         IOS_Close(sSslData->handle);
         sSslData->handle = -1;
         OSUnlockMutex(virt_addrof(sSslData->lock));
         return NSSLError::NsslLibError;
      }
   }

   OSUnlockMutex(virt_addrof(sSslData->lock));
   return NSSLError::OK;
}

NSSLError
NSSLFinish()
{
   OSLockMutex(virt_addrof(sSslData->lock));
   if (sSslData->handle != -1) {
      IOS_Close(sSslData->handle);
      sSslData->handle = -1;
   }
   OSUnlockMutex(virt_addrof(sSslData->lock));
   return NSSLError::OK;
}

NSSLContextHandle
NSSLCreateContext(ios::nsec::NSSLVersion version)
{
   if (!internal::isInitialised()) {
      return NSSLError::LibNotReady;
   }

   auto buf = internal::allocateIpcBuffer(sizeof(NSSLCreateContextRequest));
   if (!buf) {
      return NSSLError::OutOfMemory;
   }

   auto request = virt_cast<NSSLCreateContextRequest *>(buf);
   request->version = version;

   auto error = IOS_Ioctl(sSslData->handle,
                          NSSLCommand::CreateContext,
                          request,
                          sizeof(NSSLCreateContextRequest),
                          NULL,
                          0);

   internal::freeIpcBuffer(buf);
   return static_cast<NSSLError>(error);
}

namespace internal
{

static bool
isInitialised()
{
   auto initialised = true;
   OSLockMutex(virt_addrof(sSslData->lock));

   if (sSslData->handle < 0 || !sSslData->messagePool) {
      initialised = false;
   }

   OSUnlockMutex(virt_addrof(sSslData->lock));
   return initialised;
}

static virt_ptr<void>
allocateIpcBuffer(uint32_t size)
{
   auto buf = IPCBufPoolAllocate(sSslData->messagePool, size);
   if (buf) {
      std::memset(buf.get(), 0, size);
   }

   return buf;
}

static void
freeIpcBuffer(virt_ptr<void> buf)
{
   IPCBufPoolFree(sSslData->messagePool, buf);
}

void
initialiseNSSL()
{
   sSslData->handle = -1;
   OSInitMutex(virt_addrof(sSslData->lock));
}

} // namespace internal

void
Library::registerSslSymbols()
{
   RegisterFunctionExport(NSSLInit);
   RegisterFunctionExport(NSSLFinish);
   RegisterFunctionExport(NSSLCreateContext);

   RegisterDataInternal(sSslData);
}

} // namespace cafe::nsysnet
