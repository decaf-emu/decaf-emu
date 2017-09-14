#include "socket_device.h"
#include "kernel/kernel.h"

namespace ios
{

namespace dev
{

namespace socket
{

Error
SocketDevice::open(OpenMode mode)
{
   return Error::OK;
}


Error
SocketDevice::close()
{
   return Error::OK;
}


Error
SocketDevice::read(void *buffer,
                   size_t length)
{
   return Error::Invalid;
}


Error
SocketDevice::write(void *buffer,
                    size_t length)
{
   return Error::Invalid;
}


Error
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
         return static_cast<Error>(0xFFF5000B);
      }

      result = closeRequest(&request->closeRequest);
      break;
   case SocketCommand::Socket:
      result = socketRequest(&request->socketRequest);
      break;
   default:
      result = SocketError::Inval;
   }

   return static_cast<Error>(result);
}


Error
SocketDevice::ioctlv(uint32_t cmd,
                     size_t vecIn,
                     size_t vecOut,
                     IoctlVec *vec)
{
   return Error::Invalid;
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
