#pragma once
#include "socket_enum.h"
#include "socket_request.h"
#include "socket_response.h"
#include "socket_types.h"
#include "kernel/kernel_ios_device.h"

#include <cstdint>

namespace kernel
{

namespace ios
{

namespace socket
{

/**
 * \ingroup kernel_ios_socket
 * @{
 */

class SocketDevice : public IOSDevice
{
public:
   static constexpr const char *Name = "/dev/socket";

public:
   virtual IOSError
   open(IOSOpenMode mode) override;

   virtual IOSError
   close() override;

   virtual IOSError
   read(void *buffer,
        size_t length) override;

   virtual IOSError
   write(void *buffer,
         size_t length) override;

   virtual IOSError
   ioctl(uint32_t request,
         void *inBuf,
         size_t inLen,
         void *outBuf,
         size_t outLen) override;

   virtual IOSError
   ioctlv(uint32_t request,
          size_t vecIn,
          size_t vecOut,
          IOSVec *vec) override;

private:
   SocketError
   closeRequest(SocketCloseRequest *request);

   SocketError
   socketRequest(SocketSocketRequest *request);
};

/** @} */

} // namespace socket

} // namespace ios

} // namespace kernel
