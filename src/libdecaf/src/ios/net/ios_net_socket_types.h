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

struct SocketAddr
{
   be2_val<uint16_t> sa_family;
   be2_array<uint8_t, 14> sa_data;
};
CHECK_OFFSET(SocketAddr, 0x00, sa_family);
CHECK_OFFSET(SocketAddr, 0x02, sa_data);
CHECK_SIZE(SocketAddr, 0x10);

struct SockerInAddr
{
   be2_val<uint32_t> s_addr;
};
CHECK_OFFSET(SockerInAddr, 0x00, s_addr);
CHECK_SIZE(SockerInAddr, 0x04);

struct SocketAddrIn
{
   be2_val<uint16_t> sin_family;
   be2_val<uint16_t> sin_port;
   be2_struct<SockerInAddr> sin_addr;
   be2_array<uint8_t, 8> sin_zero;
};
CHECK_OFFSET(SocketAddrIn, 0x00, sin_family);
CHECK_OFFSET(SocketAddrIn, 0x02, sin_port);
CHECK_OFFSET(SocketAddrIn, 0x04, sin_addr);
CHECK_OFFSET(SocketAddrIn, 0x08, sin_zero);
CHECK_SIZE(SocketAddrIn, 0x10);

#pragma pack(pop)

/** @} */

} // namespace ios::net
