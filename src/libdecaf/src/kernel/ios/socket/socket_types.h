#pragma once
#include <cstdint>
#include <common/be_val.h>
#include <common/structsize.h>

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

#pragma pack(push, 1)

struct SockerInAddr
{
   be_val<uint32_t> s_addr;
};
CHECK_OFFSET(SockerInAddr, 0x00, s_addr);
CHECK_SIZE(SockerInAddr, 0x04);

struct SocketAddrIn
{
   be_val<uint16_t> sin_family;
   be_val<uint16_t> sin_port;
   SockerInAddr sin_addr;
   uint8_t sin_zero[8];
};
CHECK_OFFSET(SocketAddrIn, 0x00, sin_family);
CHECK_OFFSET(SocketAddrIn, 0x02, sin_port);
CHECK_OFFSET(SocketAddrIn, 0x04, sin_addr);
CHECK_OFFSET(SocketAddrIn, 0x08, sin_zero);
CHECK_SIZE(SocketAddrIn, 0x10);

#pragma pack(pop)

/** @} */

} // namespace socket

} // namespace ios

} // namespace kernel
