#ifndef SERVER_H
#define SERVER_H

#include <wut_types.h>

typedef struct Server
{
   int fd;
   int client;
   int readPos;
   int readLength;
   int readMax;
   uint8_t *readBuffer;
} Server;

typedef struct PacketReader PacketReader;
typedef struct PacketWriter PacketWriter;

typedef int (*PacketHandlerFn)(Server *server, PacketReader *reader);

int serverStart(Server *server, int port);
int serverClose(Server *server);
void serverCloseClient(Server *server);
int serverProcess(Server *server, PacketHandlerFn fn);

int serverSendPacket(Server *server, PacketWriter *packet);

#endif // SERVER_H
