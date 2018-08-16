#include "nsysnet.h"
#include "nsysnet_socket_lib.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_ghs.h"
#include "cafe/libraries/coreinit/coreinit_ios.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_ipcbufpool.h"
#include "ios/net/ios_net_socket.h"
#include "ios/ios_error.h"

namespace cafe::nsysnet
{

using namespace coreinit;

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

   be2_struct<OSMutex> lock;
   be2_val<IOSHandle> handle;
   be2_virt_ptr<IPCBufPool> messagePool;
   be2_array<uint8_t, MessageCount * MessageSize> messageBuffer;
   be2_val<uint32_t> messageCount;
};

static virt_ptr<SocketLibData>
sSocketLibData;

namespace internal
{

static bool
isInitialised();

static virt_ptr<void>
allocateIpcBuffer(uint32_t size);

static void
freeIpcBuffer(virt_ptr<void> buf);

static int32_t
decodeIosError(IOSError err);

} // namespace internal

int32_t
socket_lib_init()
{
   auto error = 0;
   OSLockMutex(virt_addrof(sSocketLibData->lock));

   if (sSocketLibData->handle < 0) {
      auto iosError = IOS_Open(make_stack_string("/dev/socket"),
                               IOSOpenMode::None);

      if (iosError < 0) {
         sSocketLibData->handle = -1;
         error = SocketError::NoLibRm;
         goto out;
      }
   }

   if (!sSocketLibData->messagePool) {
      sSocketLibData->messagePool =
         IPCBufPoolCreate(virt_addrof(sSocketLibData->messageBuffer),
                          static_cast<uint32_t>(sSocketLibData->messageBuffer.size()),
                          SocketLibData::MessageSize,
                          virt_addrof(sSocketLibData->messageCount),
                          1);

      if (!sSocketLibData->messagePool) {
         error = SocketError::NoMem;
         IOS_Close(sSocketLibData->handle);
         sSocketLibData->handle = -1;
         goto out;
      }
   }

out:
   OSUnlockMutex(virt_addrof(sSocketLibData->lock));
   gh_set_errno(error);
   return error == 0 ? 0 : -1;
}


int32_t
socket_lib_finish()
{
   auto error = 0;
   OSLockMutex(virt_addrof(sSocketLibData->lock));

   if (sSocketLibData->handle >= 0) {
      IOS_Close(sSocketLibData->handle);
      sSocketLibData->handle = -1;
   } else {
      error = SocketError::NoLibRm;
   }

   OSUnlockMutex(virt_addrof(sSocketLibData->lock));
   gh_set_errno(error);
   return error == 0 ? 0 : -1;
}


int32_t
bind(int32_t fd,
     virt_ptr<SocketAddr> addr,
     int32_t addrlen)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (!addr || addr->sa_family != SocketFamily::Inet || addrlen != sizeof(SocketAddrIn)) {
      gh_set_errno(SocketError::Inval);
      return -1;
   }

   auto buf = internal::allocateIpcBuffer(sizeof(SocketBindRequest));
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = virt_cast<SocketBindRequest *>(buf);
   request->fd = fd;
   request->addr = *virt_cast<SocketAddrIn *>(addr);
   request->addrlen = addrlen;

   auto error = IOS_Ioctl(sSocketLibData->handle,
                                    SocketCommand::Bind,
                                    request,
                                    sizeof(SocketBindRequest),
                                    NULL,
                                    0);

   auto result = internal::decodeIosError(error);

   internal::freeIpcBuffer(buf);
   return result;
}


int32_t
connect(int32_t fd,
        virt_ptr<SocketAddr> addr,
        int32_t addrlen)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   if (!addr || addr->sa_family != SocketFamily::Inet || addrlen != sizeof(SocketAddrIn)) {
      gh_set_errno(SocketError::Inval);
      return -1;
   }

   // TODO: if set_multicast_state(TRUE)

   auto buf = internal::allocateIpcBuffer(sizeof(SocketConnectRequest));
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = virt_cast<SocketConnectRequest *>(buf);
   request->fd = fd;
   request->addr = *virt_cast<SocketAddrIn *>(addr);
   request->addrlen = addrlen;

   auto error = IOS_Ioctl(sSocketLibData->handle,
                          SocketCommand::Connect,
                          request,
                          sizeof(SocketConnectRequest),
                          NULL,
                          0);

   auto result = internal::decodeIosError(error);

   internal::freeIpcBuffer(buf);
   return result;
}


int32_t
socket(int32_t family,
       int32_t type,
       int32_t proto)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   auto buf = internal::allocateIpcBuffer(sizeof(SocketSocketRequest));
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = virt_cast<SocketSocketRequest *>(buf);
   request->family = family;
   request->type = type;
   request->proto = proto;

   auto result = 0;
   auto error = IOS_Ioctl(sSocketLibData->handle,
                          SocketCommand::Socket,
                          request,
                          sizeof(SocketSocketRequest),
                          NULL,
                          0);

   if (ios::getErrorCategory(error) == ios::ErrorCategory::Socket
    && ios::getErrorCode(error) == SocketError::GenericError) {
      // Map generic socket error to no memory.
      gh_set_errno(SocketError::NoMem);
      result = -1;
   } else {
      result = internal::decodeIosError(error);
   }

   internal::freeIpcBuffer(buf);
   return result;
}


int32_t
socketclose(int32_t fd)
{
   if (!internal::isInitialised()) {
      gh_set_errno(SocketError::NotInitialised);
      return -1;
   }

   auto buf = internal::allocateIpcBuffer(sizeof(SocketCloseRequest));
   if (!buf) {
      gh_set_errno(SocketError::NoMem);
      return -1;
   }

   auto request = virt_cast<SocketCloseRequest *>(buf);
   request->fd = fd;

   auto error = IOS_Ioctl(sSocketLibData->handle,
                          SocketCommand::Close,
                          request,
                          sizeof(SocketCloseRequest),
                          NULL,
                          0);

   auto result = internal::decodeIosError(error);
   internal::freeIpcBuffer(buf);
   return result;
}

namespace internal
{

static bool
isInitialised()
{
   auto initialised = true;
   OSLockMutex(virt_addrof(sSocketLibData->lock));

   if (sSocketLibData->handle < 0 || !sSocketLibData->messagePool) {
      initialised = false;
   }

   OSUnlockMutex(virt_addrof(sSocketLibData->lock));
   return initialised;
}

static virt_ptr<void>
allocateIpcBuffer(uint32_t size)
{
   auto buf = IPCBufPoolAllocate(sSocketLibData->messagePool, size);
   if (buf) {
      std::memset(buf.getRawPointer(), 0, size);
   }

   return buf;
}

static void
freeIpcBuffer(virt_ptr<void> buf)
{
   IPCBufPoolFree(sSocketLibData->messagePool, buf);
}

static int32_t
decodeIosError(IOSError err)
{
   if (err >= 0) {
      gh_set_errno(0);
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

   gh_set_errno(error);
   return -1;
}

void
initialiseSocketLib()
{
   sSocketLibData->handle = -1;
   OSInitMutex(virt_addrof(sSocketLibData->lock));
}

} // namespace internal

void
Library::registerSocketLibSymbols()
{
   RegisterFunctionExport(socket_lib_init);
   RegisterFunctionExport(socket_lib_finish);
   RegisterFunctionExport(bind);
   RegisterFunctionExport(connect);
   RegisterFunctionExport(socket);
   RegisterFunctionExport(socketclose);

   RegisterDataInternal(sSocketLibData);
}

} // namespace cafe::nsysnet
