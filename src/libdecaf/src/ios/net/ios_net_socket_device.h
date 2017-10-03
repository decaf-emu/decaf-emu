#pragma once
#include "ios_net_enum.h"
#include "ios_net_socket_types.h"
#include "ios/ios_enum.h"

namespace ios::net::internal
{

/**
 * \ingroup ios_net
 * @{
 */

class SocketDevice
{
public:
   Error
   createSocket(int32_t family,
                int32_t type,
                int32_t proto);

   Error
   closeSocket(SocketHandle fd);
};

/** @} */

} // namespace ios::net::internal
