#include "ios/net/ios_net_socket.h"
#include "ios/ios_error.h"
#include "modules/coreinit/coreinit_ghs.h"
#include "modules/coreinit/coreinit_ios.h"
#include "modules/coreinit/coreinit_mutex.h"
#include "modules/coreinit/coreinit_ipcbufpool.h"
#include "nsysnet.h"
#include "nsysnet_socket_lib.h"

namespace nsysnet
{

using ios::net::SocketAddr;
using ios::net::SocketAddrIn;
using ios::net::SocketCommand;
using ios::net::SocketError;
using ios::net::SocketFamily;

using ios::net::SocketBindRequest;
using ios::net::SocketCloseRequest;
using ios::net::SocketConnectRequest;
using ios::net::SocketSocketRequest;

struct SocketLibData
{
   static constexpr uint32_t MessageCount = 0x20;
   static constexpr uint32_t MessageSize = 0x100;

   coreinit::OSMutex lock;
   coreinit::IOSHandle handle;
   coreinit::IPCBufPool *messagePool;
   std::array<uint8_t, MessageCount * MessageSize> messageBuffer;
   be_val<uint32_t> messageCount;
};

static SocketLibData *
sSocketLibData;

namespace internal
{

static bool
soIsInitialised()
{
   auto initialised = true;
   coreinit::OSLockMutex(&sSocketLibData->lock);

   if (sSocketLibData->handle < 0 || !sSocketLibData->messagePool) {
      initialised = false;
   }

   coreinit::OSUnlockMutex(&sSocketLibData->lock);
   return initialised;
}

static void *
soAllocateIpcBuffer(uint32_t size)
{
   auto buf = coreinit::IPCBufPoolAllocate(sSocketLibData->messagePool, size);
   if (buf) {
      std::memset(buf, 0, size);
   }

   return buf;
}

static void
soFreeIpcBuffer(void *buf)
{
   coreinit::IPCBufPoolFree(sSocketLibData->messagePool, buf);
}

static int32_t
soDecodeIosError(coreinit::IOSError err)
{
   if (err >= 0) {
      coreinit::ghs_set_errno(0);
      return err;
   }

   auto category = ios::getErrorCategory(err);
   auto code = ios::getErrorCode(err);
   auto error = SocketError::Unknown;

   switch (category) {
   case ios::ErrorCategory::Socket:
      error = static_cast<SocketError>(code);
      break;
   case ios::ErrorCategory::Kernel:
      if (code == ios::Error::Access) {
         error = SocketError::Inval;
      } else if (code == ios::Error::Intr) {
         error = SocketError::Aborted;
      } else if (code == ios::Error::QFull) {
         error = SocketError::Busy;
      } else {
         error = SocketError::Unknown;
      }
      break;
   default:
      error = SocketError::Unknown;
   }

   coreinit::ghs_set_errno(error);
   return -1;
}

} // namespace internal


int32_t
socket_lib_init()
{
   auto error = 0;
   coreinit::OSLockMutex(&sSocketLibData->lock);

   if (sSocketLibData->handle < 0) {
      auto iosError = coreinit::IOS_Open("/dev/socket", coreinit::IOSOpenMode::None);

      if (iosError < 0) {
         sSocketLibData->handle = -1;
         error = SocketError::NoLibRm;
         goto out;
      }
   }

   if (!sSocketLibData->messagePool) {
      sSocketLibData->messagePool = coreinit::IPCBufPoolCreate(sSocketLibData->messageBuffer.data(),
                                                               static_cast<uint32_t>(sSocketLibData->messageBuffer.size()),
                                                               SocketLibData::MessageSize,
                                                               &sSocketLibData->messageCount,
                                                               1);

      if (!sSocketLibData->messagePool) {
         error = SocketError::NoMem;
         coreinit::IOS_Close(sSocketLibData->handle);
         sSocketLibData->handle = -1;
         goto out;
      }
   }

out:
   coreinit::OSUnlockMutex(&sSocketLibData->lock);
   coreinit::ghs_set_errno(error);
   return error == 0 ? 0 : -1;
}


int32_t
socket_lib_finish()
{
   auto error = 0;
   coreinit::OSLockMutex(&sSocketLibData->lock);

   if (sSocketLibData->handle >= 0) {
      coreinit::IOS_Close(sSocketLibData->handle);
      sSocketLibData->handle = -1;
   } else {
      error = SocketError::NoLibRm;
   }

   coreinit::OSUnlockMutex(&sSocketLibData->lock);
   coreinit::ghs_set_errno(error);
   return error == 0 ? 0 : -1;
}


int32_t
bind(int32_t fd,
     SocketAddr *addr,
     int addrlen)
{
   if (!internal::soIsInitialised()) {
      coreinit::ghs_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (!addr || addr->sa_family != SocketFamily::Inet || addrlen != sizeof(SocketAddrIn)) {
      coreinit::ghs_set_errno(SocketError::Inval);
      return -1;
   }

   auto buf = internal::soAllocateIpcBuffer(sizeof(SocketBindRequest));
   if (!buf) {
      coreinit::ghs_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = reinterpret_cast<SocketBindRequest *>(buf);
   request->fd = fd;
   request->addr = *reinterpret_cast<SocketAddrIn *>(addr);
   request->addrlen = addrlen;

   auto error = coreinit::IOS_Ioctl(sSocketLibData->handle,
                                    SocketCommand::Bind,
                                    request,
                                    sizeof(SocketBindRequest),
                                    NULL,
                                    0);

   auto result = internal::soDecodeIosError(error);

   internal::soFreeIpcBuffer(buf);
   return result;
}


int32_t
connect(int32_t fd,
        SocketAddr *addr,
        int addrlen)
{
   if (!internal::soIsInitialised()) {
      coreinit::ghs_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (!addr || addr->sa_family != SocketFamily::Inet || addrlen != sizeof(SocketAddrIn)) {
      coreinit::ghs_set_errno(SocketError::Inval);
      return -1;
   }

   // TODO: if set_multicast_state(TRUE)

   auto buf = internal::soAllocateIpcBuffer(sizeof(SocketConnectRequest));
   if (!buf) {
      coreinit::ghs_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = reinterpret_cast<SocketConnectRequest *>(buf);
   request->fd = fd;
   request->addr = *reinterpret_cast<SocketAddrIn *>(addr);
   request->addrlen = addrlen;

   auto error = coreinit::IOS_Ioctl(sSocketLibData->handle,
                                    SocketCommand::Connect,
                                    request,
                                    sizeof(SocketConnectRequest),
                                    NULL,
                                    0);

   auto result = internal::soDecodeIosError(error);

   internal::soFreeIpcBuffer(buf);
   return result;
}


int32_t
socket(int32_t family,
       int32_t type,
       int32_t proto)
{
   if (!internal::soIsInitialised()) {
      coreinit::ghs_set_errno(SocketError::NotInitialised);
      return -1;
   }

   auto buf = internal::soAllocateIpcBuffer(sizeof(SocketSocketRequest));
   if (!buf) {
      coreinit::ghs_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = reinterpret_cast<SocketSocketRequest *>(buf);
   request->family = family;
   request->type = type;
   request->proto = proto;

   auto result = 0;
   auto error = coreinit::IOS_Ioctl(sSocketLibData->handle,
                                    SocketCommand::Socket,
                                    request,
                                    sizeof(SocketSocketRequest),
                                    NULL,
                                    0);

   if (ios::getErrorCategory(error) == ios::ErrorCategory::Socket
    && ios::getErrorCode(error) == SocketError::GenericError) {
      // Map generic socket error to no memory.
      coreinit::ghs_set_errno(SocketError::NoMem);
      result = -1;
   } else {
      result = internal::soDecodeIosError(error);
   }

   internal::soFreeIpcBuffer(buf);
   return result;
}


int32_t
socketclose(int32_t fd)
{
   if (!internal::soIsInitialised()) {
      coreinit::ghs_set_errno(SocketError::NotInitialised);
      return -1;
   }

   auto buf = internal::soAllocateIpcBuffer(sizeof(SocketCloseRequest));
   if (!buf) {
      coreinit::ghs_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = reinterpret_cast<SocketCloseRequest *>(buf);
   request->fd = fd;

   auto error = coreinit::IOS_Ioctl(sSocketLibData->handle,
                                    SocketCommand::Close,
                                    request,
                                    sizeof(SocketCloseRequest),
                                    NULL,
                                    0);

   auto result = internal::soDecodeIosError(error);
   internal::soFreeIpcBuffer(buf);
   return result;
}


void
Module::registerSocketLibFunctions()
{
   RegisterKernelFunction(socket_lib_init);
   RegisterKernelFunction(socket_lib_finish);
   RegisterKernelFunction(bind);
   RegisterKernelFunction(connect);
   RegisterKernelFunction(socket);
   RegisterKernelFunction(socketclose);

   RegisterInternalData(sSocketLibData);
}

void
Module::initialiseSocketLib()
{
   sSocketLibData->handle = -1;
   coreinit::OSInitMutex(&sSocketLibData->lock);
}

} // namespace nsysnet
