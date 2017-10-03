#include "ios_net_socket_device.h"

namespace ios::net::internal
{

Error
SocketDevice::createSocket(int32_t family,
                           int32_t type,
                           int32_t proto)
{
   return Error::InvalidArg;
}

Error
SocketDevice::closeSocket(SocketHandle fd)
{
   return Error::InvalidArg;
}

} // namespace ios::net::internal
