#pragma once
#include <libcpu/be2_struct.h>
#include <common/structsize.h>

namespace ios::net
{

/**
 * \ingroup ios_net
 * @{
 */

#pragma pack(push, 1)

using SocketHandle = int32_t;
using SocketFdSet = uint32_t;

struct SocketAddr
{
   be2_val<uint16_t> sa_family;
   be2_array<uint8_t, 14> sa_data;
};
CHECK_OFFSET(SocketAddr, 0x00, sa_family);
CHECK_OFFSET(SocketAddr, 0x02, sa_data);
CHECK_SIZE(SocketAddr, 0x10);

struct SocketInAddr
{
   be2_val<uint32_t> s_addr_;
};
CHECK_OFFSET(SocketInAddr, 0x00, s_addr_);
CHECK_SIZE(SocketInAddr, 0x04);

struct SocketHostEnt
{
   be2_phys_ptr<char> h_name;
   be2_phys_ptr<char> h_aliases;
   be2_val<int32_t> h_addrtype;
   be2_val<int32_t> h_length;
   be2_phys_ptr<char> h_addr_list;
};
CHECK_OFFSET(SocketHostEnt, 0x00, h_name);
CHECK_OFFSET(SocketHostEnt, 0x04, h_aliases);
CHECK_OFFSET(SocketHostEnt, 0x08, h_addrtype);
CHECK_OFFSET(SocketHostEnt, 0x0C, h_length);
CHECK_OFFSET(SocketHostEnt, 0x10, h_addr_list);
CHECK_SIZE(SocketHostEnt, 0x14);

struct SocketAddrIn
{
   be2_val<uint16_t> sin_family;
   be2_val<uint16_t> sin_port;
   be2_struct<SocketInAddr> sin_addr;
   be2_array<uint8_t, 8> sin_zero;
};
CHECK_OFFSET(SocketAddrIn, 0x00, sin_family);
CHECK_OFFSET(SocketAddrIn, 0x02, sin_port);
CHECK_OFFSET(SocketAddrIn, 0x04, sin_addr);
CHECK_OFFSET(SocketAddrIn, 0x08, sin_zero);
CHECK_SIZE(SocketAddrIn, 0x10);

struct SocketTimeval
{
   be2_val<int32_t> tv_sec;
   be2_val<int32_t> tv_usec;
};
CHECK_OFFSET(SocketTimeval, 0x00, tv_sec);
CHECK_OFFSET(SocketTimeval, 0x04, tv_usec);
CHECK_SIZE(SocketTimeval, 0x08);

#pragma pack(pop)

/** @} */

} // namespace ios::net
