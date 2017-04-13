#include "platform_socket.h"

#ifdef PLATFORM_POSIX
#include <errno.h>
#include <unistd.h>

namespace platform
{

bool socketWouldBlock(int result)
{
   return (result == EWOULDBLOCK);
}

int socketSetBlocking(Socket socket, bool blocking)
{
   auto fl = fcntl(socket, F_GETFL, 0);

   if (blocking) {
      fl &= ~O_NONBLOCK;
   } else {
      fl |= O_NONBLOCK;
   }

   return fcntl(socket, F_SETFL, fl);
}

int socketClose(Socket socket)
{
   return close(socket);
}

} // namespace platform
#endif
