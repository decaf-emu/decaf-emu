#pragma once
#include "ios/dev/ios_device.h"
#include "socket_enum.h"
#include "socket_request.h"
#include "socket_response.h"
#include "socket_types.h"

#include <cstdint>

namespace ios
{

namespace dev
{

namespace socket
{

/**
 * \defgroup ios_dev_socket /dev/socket
 * \ingroup ios_dev
 * @{
 */

class SocketDevice : public IOSDevice
{
public:
   static constexpr const char *Name = "/dev/socket";

public:
   virtual Error
   open(OpenMode mode) override;

   virtual Error
   close() override;

   virtual Error
   read(void *buffer,
        size_t length) override;

   virtual Error
   write(void *buffer,
         size_t length) override;

   virtual Error
   ioctl(uint32_t request,
         void *inBuf,
         size_t inLen,
         void *outBuf,
         size_t outLen) override;

   virtual Error
   ioctlv(uint32_t request,
          size_t vecIn,
          size_t vecOut,
          IoctlVec *vec) override;

private:
   SocketError
   closeRequest(SocketCloseRequest *request);

   SocketError
   socketRequest(SocketSocketRequest *request);
};

/** @} */

} // namespace socket

} // namespace dev

} // namespace ios
