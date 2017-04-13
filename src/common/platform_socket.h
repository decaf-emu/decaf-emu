#pragma once
#include "platform.h"

#ifdef PLATFORM_WINDOWS
#include <WinSock2.h>
#include <Ws2tcpip.h>
#else
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

namespace platform
{

#ifdef PLATFORM_WINDOWS
using Socket = SOCKET;
#else
using Socket = int;
#endif

bool socketWouldBlock(int result);
int socketSetBlocking(Socket socket, bool blocking);
int socketClose(Socket socket);

} // namespace platform
