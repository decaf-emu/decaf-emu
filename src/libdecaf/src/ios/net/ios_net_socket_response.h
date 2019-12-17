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

struct SocketAcceptResponse
{
   be2_val<SocketHandle> fd;
   be2_struct<SocketAddrIn> addr;
   be2_val<int32_t> addrlen;
};
CHECK_OFFSET(SocketAcceptResponse, 0x00, fd);
CHECK_OFFSET(SocketAcceptResponse, 0x04, addr);
CHECK_OFFSET(SocketAcceptResponse, 0x14, addrlen);
CHECK_SIZE(SocketAcceptResponse, 0x18);

struct SocketDnsQueryResponse
{
   be2_val<uint32_t> unk0x00;
   be2_val<uint32_t> unk0x04;
   be2_val<uint32_t> sendTime;
   be2_val<uint32_t> expireTime;
   be2_val<uint16_t> tries;
   be2_val<uint16_t> lport;
   be2_val<uint16_t> id;
   UNKNOWN(0x2);
   be2_val<uint32_t> unk0x18;
   be2_val<uint32_t> replies;
   be2_val<uint32_t> ipaddrs;
   be2_array<uint32_t, 10> ipaddrList;
   be2_array<uint32_t, 10> hostentIpaddrList;
   be2_val<uint32_t> err;
   be2_val<uint32_t> rcode;
   be2_array<char, 256> dnsNames;
   be2_array<char, 129> unk0x17C;
   UNKNOWN(0x27C - 0x1FD);
   be2_val<uint32_t> authsIp;
   be2_array<uint32_t, 2> aliases;
   UNKNOWN(0x290 - 0x288);
   be2_struct<SocketHostEnt> hostent;
   be2_val<SocketDnsQueryType> queryType;
   be2_array<uint8_t, 2> unk0x2A5;
   UNKNOWN(0x2B0 - 0x2A7);
   be2_val<uint32_t> dnsReq;
   be2_val<uint32_t> next;

   //! Used to adjust pointers in hostent
   be2_val<uint32_t> selfPointerOffset;
};
CHECK_OFFSET(SocketDnsQueryResponse, 0x00, unk0x00);
CHECK_OFFSET(SocketDnsQueryResponse, 0x04, unk0x04);
CHECK_OFFSET(SocketDnsQueryResponse, 0x08, sendTime);
CHECK_OFFSET(SocketDnsQueryResponse, 0x0C, expireTime);
CHECK_OFFSET(SocketDnsQueryResponse, 0x10, tries);
CHECK_OFFSET(SocketDnsQueryResponse, 0x12, lport);
CHECK_OFFSET(SocketDnsQueryResponse, 0x14, id);
CHECK_OFFSET(SocketDnsQueryResponse, 0x18, unk0x18);
CHECK_OFFSET(SocketDnsQueryResponse, 0x1C, replies);
CHECK_OFFSET(SocketDnsQueryResponse, 0x20, ipaddrs);
CHECK_OFFSET(SocketDnsQueryResponse, 0x24, ipaddrList);
CHECK_OFFSET(SocketDnsQueryResponse, 0x4C, hostentIpaddrList);
CHECK_OFFSET(SocketDnsQueryResponse, 0x74, err);
CHECK_OFFSET(SocketDnsQueryResponse, 0x78, rcode);
CHECK_OFFSET(SocketDnsQueryResponse, 0x7C, dnsNames);
CHECK_OFFSET(SocketDnsQueryResponse, 0x17C, unk0x17C);
CHECK_OFFSET(SocketDnsQueryResponse, 0x27C, authsIp);
CHECK_OFFSET(SocketDnsQueryResponse, 0x280, aliases);
CHECK_OFFSET(SocketDnsQueryResponse, 0x290, hostent);
CHECK_OFFSET(SocketDnsQueryResponse, 0x2A4, queryType);
CHECK_OFFSET(SocketDnsQueryResponse, 0x2A5, unk0x2A5);
CHECK_OFFSET(SocketDnsQueryResponse, 0x2B0, dnsReq);
CHECK_OFFSET(SocketDnsQueryResponse, 0x2B4, next);
CHECK_OFFSET(SocketDnsQueryResponse, 0x2B8, selfPointerOffset);
CHECK_SIZE(SocketDnsQueryResponse, 0x2BC);

struct SocketSelectResponse
{
   be2_val<int32_t> nfds;
   be2_val<SocketFdSet> readfds;
   be2_val<SocketFdSet> writefds;
   be2_val<SocketFdSet> exceptfds;
   be2_struct<SocketTimeval> timeout;
   be2_val<int32_t> hasTimeout;
};
CHECK_OFFSET(SocketSelectResponse, 0x00, nfds);
CHECK_OFFSET(SocketSelectResponse, 0x04, readfds);
CHECK_OFFSET(SocketSelectResponse, 0x08, writefds);
CHECK_OFFSET(SocketSelectResponse, 0x0C, exceptfds);
CHECK_OFFSET(SocketSelectResponse, 0x10, timeout);
CHECK_OFFSET(SocketSelectResponse, 0x18, hasTimeout);
CHECK_SIZE(SocketSelectResponse, 0x1C);

struct SocketResponse
{
   union
   {
      be2_struct<SocketAcceptResponse> accept;
      be2_struct<SocketDnsQueryResponse> dnsQuery;
      be2_struct<SocketSelectResponse> select;
   };
};

#pragma pack(pop)

/** @} */

} // namespace ios::net
