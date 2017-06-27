#pragma once
#include "socket_enum.h"
#include "socket_types.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

namespace ios
{

namespace dev
{

namespace socket
{

/**
 * \ingroup ios_dev_socket
 * @{
 */

#pragma pack(push, 1)

struct SocketBindRequest
{
   be_val<int32_t> fd;
   SocketAddrIn addr;
   be_val<int32_t> addrlen;
};
CHECK_OFFSET(SocketBindRequest, 0x00, fd);
CHECK_OFFSET(SocketBindRequest, 0x04, addr);
CHECK_OFFSET(SocketBindRequest, 0x14, addrlen);
CHECK_SIZE(SocketBindRequest, 0x18);

struct SocketCloseRequest
{
   be_val<int32_t> fd;
};
CHECK_OFFSET(SocketCloseRequest, 0x00, fd);
CHECK_SIZE(SocketCloseRequest, 0x04);

struct SocketConnectRequest
{
   be_val<int32_t> fd;
   SocketAddrIn addr;
   be_val<int32_t> addrlen;
};
CHECK_OFFSET(SocketConnectRequest, 0x00, fd);
CHECK_OFFSET(SocketConnectRequest, 0x04, addr);
CHECK_OFFSET(SocketConnectRequest, 0x14, addrlen);
CHECK_SIZE(SocketConnectRequest, 0x18);

struct SocketSocketRequest
{
   be_val<int32_t> family;
   be_val<int32_t> type;
   be_val<int32_t> proto;
};
CHECK_OFFSET(SocketSocketRequest, 0x00, family);
CHECK_OFFSET(SocketSocketRequest, 0x04, type);
CHECK_OFFSET(SocketSocketRequest, 0x08, proto);
CHECK_SIZE(SocketSocketRequest, 0x0C);

struct SocketRequest
{
   union
   {
      SocketCloseRequest bindRequest;
      SocketCloseRequest closeRequest;
      SocketConnectRequest connectRequest;
      SocketSocketRequest socketRequest;
   };
};

#pragma pack(pop)

/** @} */

} // namespace im

} // namespace dev

} // namespace ios
