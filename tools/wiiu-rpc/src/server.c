#include "server.h"
#include "console.h"
#include "packet.h"

#include <coreinit/memexpheap.h>
#include <coreinit/memheap.h>
#include <nn/ac/ac_c.h>
#include <nsysnet/socket.h>
#include <stdbool.h>
#include <stdbool.h>
#include <string.h>
#include <whb/log.h>

int
serverStart(Server *server, int port)
{
   int fd, opt;
   struct sockaddr_in addr;
   struct in_addr ip_addr;
   uint32_t ipAddress;
   ACConfigId startupId;
   MEMHeapHandle mem2 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);

   server->fd = -1;
   server->client = -1;
   server->readPos = 0;
   server->readLength = 0;
   server->readMax = 1024 * 1024;
   server->readBuffer = (uint8_t *)MEMAllocFromExpHeapEx(mem2, server->readMax, 0x100);

   fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

   if (fd < 0) {
      WHBLogPrintf("Error creating socket: %d", socketlasterr());
      return -1;
   }

   setsockopt(fd, SOL_SOCKET, SO_NBIO, NULL, 0);

   opt = 1;
   setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = htonl(INADDR_ANY);

   if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      socketclose(fd);
      WHBLogPrintf("Error binding socket: %d", socketlasterr());
      return -1;
   }

   if (listen(fd, 3) < 0) {
      socketclose(fd);
      WHBLogPrintf("Error listening on socket: %d", socketlasterr());
      return -1;
   }

   ACGetStartupId(&startupId);
   ACConnectWithConfigId(startupId);
   ACGetAssignedAddress(&ipAddress);

   ip_addr.s_addr = ipAddress;
   WHBLogPrintf("Started server on %s:%d", inet_ntoa(ip_addr), port);
   server->fd = fd;
   return 0;
}

int serverClose(Server *server)
{
   MEMHeapHandle mem2 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);
   serverCloseClient(server);

   if (server->fd != -1) {
      socketclose(server->fd);
      server->fd = -1;
   }

   server->readMax = 0;
   MEMFreeToExpHeap(mem2, server->readBuffer);
   return 0;
}

void
serverCloseClient(Server *server)
{
   if (server->client != -1) {
      socketclose(server->client);
      server->client = -1;
   }
}

static int
serverAccept(Server *server)
{
   struct sockaddr_in addr;
   socklen_t addrlen = sizeof(addr);
   int clientFd;

   while (true) {
      clientFd = accept(server->fd, (struct sockaddr *)&addr, &addrlen);

      if (clientFd < 0) {
         int error = socketlasterr();

         if (error != NSN_EWOULDBLOCK) {
            WHBLogPrintf("Error, accept returned %d", clientFd);
            return error;
         }

         return 0;
      }

      if (server->client != -1) {
         WHBLogPrintf("Max clients reached, reject %s", inet_ntoa(addr.sin_addr));
         socketclose(clientFd);
         return 0;
      }

      WHBLogPrintf("Accepted connection from %s", inet_ntoa(addr.sin_addr));
      server->client = clientFd;
   }

   return 0;
}

int serverSendPacket(Server *server, PacketWriter *packet)
{
   return send(server->client, packet->buffer, packet->pos, 0);
}

static int
serverHandlePacket(Server *server, PacketHandlerFn fn)
{
   PacketReader reader;
   reader.dataLength = *(uint32_t *)(server->readBuffer + 0) - 8;
   reader.command = *(uint32_t *)(server->readBuffer + 4);
   reader.data = server->readBuffer + 8;
   reader.pos = 0;

   return fn(server, &reader);
}

static int
serverRead(Server *server, PacketHandlerFn fn)
{
   while (server->client != -1) {
      int read;
      int length;

      if (server->readLength == 0) {
         length = 4;
      } else {
         length = server->readLength;
      }

      read = recv(server->client, server->readBuffer + server->readPos, length, 0);
      if (read < 0) {
         int error = socketlasterr();

         if (error != NSN_EWOULDBLOCK) {
            WHBLogPrintf("Error receiving from socket: %d", error);
            serverCloseClient(server);
            return error;
         }

         return 0;
      } else if (read == 0) {
         WHBLogPrintf("Client disconnected gracefully.");
         serverCloseClient(server);
         return read;
      }

      server->readPos += read;

      if (server->readLength == 0 && server->readPos >= 2) {
         server->readLength = *(uint32_t *)server->readBuffer;
      }

      if (server->readPos >= server->readLength) {
         serverHandlePacket(server, fn);
         server->readPos = 0;
         server->readLength = 0;
      }
   }

   return 0;
}

int
serverProcess(Server *server, PacketHandlerFn fn)
{
   int error = serverAccept(server);

   if (error) {
      return error;
   }

   return serverRead(server, fn);
}
