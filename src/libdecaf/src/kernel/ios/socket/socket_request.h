#pragma once
#include "socket_enum.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
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

struct SocketCloseRequest
{
   be_val<int32_t> fd;
};
CHECK_OFFSET(SocketCloseRequest, 0x00, fd);
CHECK_SIZE(SocketCloseRequest, 0x04);

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
      SocketCloseRequest closeRequest;
      SocketSocketRequest socketRequest;
   };
};

#pragma pack(pop)

/** @} */

} // namespace im

} // namespace ios

} // namespace kernel
