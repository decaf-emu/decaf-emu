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
using ios::nsec::NSSLDestroyContextRequest;
using ios::nsec::NSSLAddServerPKIRequest;
using ios::nsec::NSSLAddServerPKIExternalRequest;

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

NSSLError
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

NSSLError
NSSLDestroyContext(NSSLContextHandle context)
{
   if (!internal::isInitialised()) {
      return NSSLError::LibNotReady;
   }

   auto buf = internal::allocateIpcBuffer(sizeof(NSSLDestroyContextRequest));
   if (!buf) {
      return NSSLError::OutOfMemory;
   }

   auto request = virt_cast<NSSLDestroyContextRequest *>(buf);
   request->context = context;

   auto error = IOS_Ioctl(sSslData->handle,
                          NSSLCommand::DestroyContext,
                          request,
                          sizeof(NSSLDestroyContextRequest),
                          NULL,
                          0);

   internal::freeIpcBuffer(buf);
   return static_cast<NSSLError>(error);
}

NSSLError
NSSLAddServerPKI(NSSLContextHandle context,
                 NSSLCertID certId)
{
   if (!internal::isInitialised()) {
      return NSSLError::LibNotReady;
   }

   auto buf = internal::allocateIpcBuffer(sizeof(NSSLAddServerPKIRequest));
   if (!buf) {
      return NSSLError::OutOfMemory;
   }

   auto request = virt_cast<NSSLAddServerPKIRequest *>(buf);
   request->context = context;
   request->cert = certId;

   auto error = IOS_Ioctl(sSslData->handle,
                          NSSLCommand::AddServerPKI,
                          request,
                          sizeof(NSSLAddServerPKIRequest),
                          NULL,
                          0);

   internal::freeIpcBuffer(buf);
   return static_cast<NSSLError>(error);
}

NSSLError
NSSLAddServerPKIExternal(NSSLContextHandle context,
                         virt_ptr<uint8_t> cert,
                         uint32_t certSize,
                         uint32_t unkArg4)
{
   if (!internal::isInitialised()) {
      return NSSLError::LibNotReady;
   }

   auto buf = internal::allocateIpcBuffer(0x88);
   if (!buf) {
      return NSSLError::OutOfMemory;
   }

   auto vec = virt_cast<IOSVec *>(buf);
   vec[0].vaddr = virt_cast<virt_addr>(cert);
   vec[0].len = certSize;

   auto request = virt_cast<NSSLAddServerPKIExternalRequest *>(virt_cast<virt_addr>(buf) + 0x80);
   vec[1].vaddr = virt_cast<virt_addr>(request);
   vec[1].len = static_cast<uint32_t>(sizeof(NSSLAddServerPKIExternalRequest));

   request->context = context;
   request->certType = 0;

   auto error = IOS_Ioctlv(sSslData->handle,
                           NSSLCommand::AddServerPKIExternal,
                           2, 0, vec);

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
   RegisterFunctionExport(NSSLDestroyContext);
   RegisterFunctionExport(NSSLAddServerPKI);
   RegisterFunctionExport(NSSLAddServerPKIExternal);

   RegisterDataInternal(sSslData);
}

} // namespace cafe::nsysnet
