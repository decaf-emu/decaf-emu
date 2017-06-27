#pragma once
#include "ios/dev/socket.h"
#include <cstdint>

namespace nsysnet
{

using ios::dev::socket::SocketAddr;

int32_t
socket_lib_init();

int32_t
socket_lib_finish();

int32_t
bind(int32_t fd,
     SocketAddr *addr,
     int addrlen);

int32_t
connect(int32_t fd,
        SocketAddr *addr,
        int addrlen);

int32_t
socket(int32_t family,
       int32_t type,
       int32_t proto);

int32_t
socketclose(int32_t fd);

} // namespace nsysnet
