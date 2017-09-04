#include "socket_device.h"
#include "kernel/kernel.h"

namespace ios
{

namespace dev
{

namespace socket
{

IOSError
SocketDevice::open(IOSOpenMode mode)
{
   return IOSError::OK;
}


IOSError
SocketDevice::close()
{
   return IOSError::OK;
}


IOSError
SocketDevice::read(void *buffer,
                   size_t length)
{
   return IOSError::Invalid;
}


IOSError
SocketDevice::write(void *buffer,
                    size_t length)
{
   return IOSError::Invalid;
}


IOSError
SocketDevice::ioctl(uint32_t cmd,
                    void *inBuf,
                    size_t inLen,
                    void *outBuf,
                    size_t outLen)
{
   auto request = reinterpret_cast<SocketRequest *>(inBuf);
   auto result = SocketError::OK;

   switch (static_cast<SocketCommand>(cmd)) {
   case SocketCommand::Close:
      if (inLen != 4) {
         return static_cast<IOSError>(0xFFF5000B);
      }

      result = closeRequest(&request->closeRequest);
      break;
   case SocketCommand::Socket:
      result = socketRequest(&request->socketRequest);
      break;
   default:
      result = SocketError::Inval;
   }

   return static_cast<IOSError>(result);
}


IOSError
SocketDevice::ioctlv(uint32_t cmd,
                     size_t vecIn,
                     size_t vecOut,
                     IOSVec *vec)
{
   return IOSError::Invalid;
}


SocketError
SocketDevice::closeRequest(SocketCloseRequest *request)
{
   // TODO: close(fd)
   return SocketError::Inval;
}


SocketError
SocketDevice::socketRequest(SocketSocketRequest *request)
{
   // TODO: socket(family, proto, type)
   return SocketError::Inval;
}

} // namespace socket

} // namespace dev

} // namespace ios
