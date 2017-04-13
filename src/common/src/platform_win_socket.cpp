#include "platform_socket.h"

#ifdef PLATFORM_WINDOWS
namespace platform
{

bool socketWouldBlock(int result)
{
   return (result < 0 && WSAGetLastError() == WSAEWOULDBLOCK);
}

int socketSetBlocking(Socket socket, bool blocking)
{
   u_long iMode = blocking ? 0 : 1;
   return ioctlsocket(socket, FIONBIO, &iMode);
}

int socketClose(Socket socket)
{
   return closesocket(socket);
}

} // namespace platform
#endif
