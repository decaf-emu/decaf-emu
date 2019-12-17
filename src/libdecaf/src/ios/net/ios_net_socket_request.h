#pragma once
#include "ios_net_enum.h"
#include "ios_net_socket_types.h"

#include "ios/ios_ipc.h"

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

struct SocketAcceptRequest
{
   be2_val<SocketHandle> fd;
   be2_struct<SocketAddrIn> addr;
   be2_val<int32_t> addrlen;
};
CHECK_OFFSET(SocketAcceptRequest, 0x00, fd);
CHECK_OFFSET(SocketAcceptRequest, 0x04, addr);
CHECK_OFFSET(SocketAcceptRequest, 0x14, addrlen);
CHECK_SIZE(SocketAcceptRequest, 0x18);

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

struct SocketDnsQueryRequest
{
   be2_array<char, 0x80> name;
   be2_val<SocketDnsQueryType> queryType;
   be2_val<uint8_t> isAsync;
   PADDING(2);
   UNKNOWN(4);
   be2_val<uint32_t> unk0x88;
   be2_val<uint32_t> unk0x8C;
};
CHECK_OFFSET(SocketDnsQueryRequest, 0x00, name);
CHECK_OFFSET(SocketDnsQueryRequest, 0x80, queryType);
CHECK_OFFSET(SocketDnsQueryRequest, 0x81, isAsync);
CHECK_OFFSET(SocketDnsQueryRequest, 0x88, unk0x88);
CHECK_OFFSET(SocketDnsQueryRequest, 0x8C, unk0x8C);
CHECK_SIZE(SocketDnsQueryRequest, 0x90);

struct SocketGetPeerNameRequest
{
   be2_val<SocketHandle> fd;
   be2_struct<SocketAddrIn> addr;
   be2_val<int32_t> addrlen;
};
CHECK_OFFSET(SocketGetPeerNameRequest, 0x00, fd);
CHECK_OFFSET(SocketGetPeerNameRequest, 0x04, addr);
CHECK_OFFSET(SocketGetPeerNameRequest, 0x14, addrlen);
CHECK_SIZE(SocketGetPeerNameRequest, 0x18);

struct SocketGetProcessSocketHandle
{
   be2_val<ios::TitleId> titleId;
   be2_val<ios::ProcessId> processId;
};
CHECK_OFFSET(SocketGetProcessSocketHandle, 0x00, titleId);
CHECK_OFFSET(SocketGetProcessSocketHandle, 0x08, processId);
CHECK_SIZE(SocketGetProcessSocketHandle, 0x0C);

struct SocketListenRequest
{
   be2_val<SocketHandle> fd;
   be2_val<int32_t> backlog;
};
CHECK_OFFSET(SocketListenRequest, 0x00, fd);
CHECK_OFFSET(SocketListenRequest, 0x04, backlog);
CHECK_SIZE(SocketListenRequest, 0x08);

struct SocketRecvRequest
{
   be2_val<SocketHandle> fd;
   be2_val<int32_t> flags;
};
CHECK_OFFSET(SocketRecvRequest, 0x00, fd);
CHECK_OFFSET(SocketRecvRequest, 0x04, flags);
CHECK_SIZE(SocketRecvRequest, 0x08);

struct SocketSelectRequest
{
   be2_val<int32_t> nfds;
   be2_val<SocketFdSet> readfds;
   be2_val<SocketFdSet> writefds;
   be2_val<SocketFdSet> exceptfds;
   be2_struct<SocketTimeval> timeout;
   be2_val<int32_t> hasTimeout;
};
CHECK_OFFSET(SocketSelectRequest, 0x00, nfds);
CHECK_OFFSET(SocketSelectRequest, 0x04, readfds);
CHECK_OFFSET(SocketSelectRequest, 0x08, writefds);
CHECK_OFFSET(SocketSelectRequest, 0x0C, exceptfds);
CHECK_OFFSET(SocketSelectRequest, 0x10, timeout);
CHECK_OFFSET(SocketSelectRequest, 0x18, hasTimeout);
CHECK_SIZE(SocketSelectRequest, 0x1C);

struct SocketSetSockOptRequest
{
   // For some reason this structure overlaps the ioctlv vecs...
   PADDING(0xC * 2);
   be2_val<SocketHandle> fd;
   be2_val<int32_t> level;
   be2_val<int32_t> optname;
};
CHECK_OFFSET(SocketSetSockOptRequest, 0x18, fd);
CHECK_OFFSET(SocketSetSockOptRequest, 0x1C, level);
CHECK_OFFSET(SocketSetSockOptRequest, 0x20, optname);
CHECK_SIZE(SocketSetSockOptRequest, 0x24);

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
      be2_struct<SocketAcceptRequest> accept;
      be2_struct<SocketBindRequest> bind;
      be2_struct<SocketCloseRequest> close;
      be2_struct<SocketConnectRequest> connect;
      be2_struct<SocketDnsQueryRequest> dnsQuery;
      be2_struct<SocketGetPeerNameRequest> getpeername;
      be2_struct<SocketListenRequest> listen;
      be2_struct<SocketRecvRequest> recv;
      be2_struct<SocketSelectRequest> select;
      be2_struct<SocketSetSockOptRequest> setsockopt;
      be2_struct<SocketSocketRequest> socket;
      be2_struct<SocketGetProcessSocketHandle> getProcessSocketHandle;
   };
};

#pragma pack(pop)

/** @} */

} // namespace ios::net
