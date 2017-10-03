#pragma once
#include "ios_net_enum.h"
#include "ios_net_socket_types.h"

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <common/structsize.h>

namespace ios::net
{

/**
 * \ingroup ios_net
 * @{
 */

#pragma pack(push, 1)

struct SocketBindRequest
{
   be2_val<SocketHandle> fd;
   be2_struct<SocketAddrIn> addr;
   be2_val<int32_t> addrlen;
};
CHECK_OFFSET(SocketBindRequest, 0x00, fd);
CHECK_OFFSET(SocketBindRequest, 0x04, addr);
CHECK_OFFSET(SocketBindRequest, 0x14, addrlen);
CHECK_SIZE(SocketBindRequest, 0x18);

struct SocketCloseRequest
{
   be2_val<SocketHandle> fd;
};
CHECK_OFFSET(SocketCloseRequest, 0x00, fd);
CHECK_SIZE(SocketCloseRequest, 0x04);

struct SocketConnectRequest
{
   be2_val<SocketHandle> fd;
   be2_struct<SocketAddrIn> addr;
   be2_val<int32_t> addrlen;
};
CHECK_OFFSET(SocketConnectRequest, 0x00, fd);
CHECK_OFFSET(SocketConnectRequest, 0x04, addr);
CHECK_OFFSET(SocketConnectRequest, 0x14, addrlen);
CHECK_SIZE(SocketConnectRequest, 0x18);

struct SocketSocketRequest
{
   be2_val<int32_t> family;
   be2_val<int32_t> type;
   be2_val<int32_t> proto;
};
CHECK_OFFSET(SocketSocketRequest, 0x00, family);
CHECK_OFFSET(SocketSocketRequest, 0x04, type);
CHECK_OFFSET(SocketSocketRequest, 0x08, proto);
CHECK_SIZE(SocketSocketRequest, 0x0C);

struct SocketRequest
{
   union
   {
      be2_struct<SocketCloseRequest> bind;
      be2_struct<SocketCloseRequest> close;
      be2_struct<SocketConnectRequest> connect;
      be2_struct<SocketSocketRequest> socket;
   };
};

#pragma pack(pop)

/** @} */

} // namespace ios::net
