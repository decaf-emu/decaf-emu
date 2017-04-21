#include "program.h"
#include "console.h"
#include "sysfuncs.h"

#define CLIENT_VERSION 1

#define ALIGN_BACKWARD(x,align) \
   ((typeof(x))(((unsigned int)(x)) & (~(align-1))))

int sendwait(struct SystemFunctions *sysFuncs, int sock, const void *buffer, int len);
int recvwait(struct SystemFunctions *sysFuncs, int sock, void *buffer, int len);

struct PacketHeader
{
   uint16_t size;
   uint16_t command;
};

struct VersionPacket
{
   struct PacketHeader header;
   uint32_t version;
};

struct ExecuteCodeTestPacket
{
   struct PacketHeader header;
   uint32_t instr;
   struct TestState state;
};

void writeInstruction(struct SystemFunctions *sysFuncs, void *func, uint32_t instr)
{
   // Write code
   uint32_t *topatch = (uint32_t*)(0xA0000000 + (uint32_t)func);
   topatch[0] = instr;        // instr
   topatch[1] = 0x4E800020;   // blr

   // Flush caches
   unsigned int *faddr = ALIGN_BACKWARD(topatch, 32);
   sysFuncs->DCFlushRange(faddr, 0x40);
   sysFuncs->ICInvalidateRange(faddr, 0x40);
}

void _entryPoint()
{
   struct SystemFunctions sysFuncs;
   struct ConsoleData consoleData;

   loadSysFuncs(&sysFuncs);
   allocConsole(&sysFuncs, &consoleData);

   char *packetBuffer = (char *)sysFuncs.OSAllocFromSystem(4096, 16);
   sysFuncs.socket_lib_init();

   int error;
   VPADData vpad_data;
   sysFuncs.VPADRead(0, &vpad_data, 1, &error);

   struct sockaddr sin;
   sysFuncs.memset(&sin, 0, sizeof(struct sockaddr));
   sin.sin_family = AF_INET;
   sin.sin_port = 8008;
   sin.sin_addr.s_addr = ((192<<24) | (168<<16) | (1<<8) | (67<<0));
   LOG(&consoleData, "Create socket");
   int sockfd = sysFuncs.socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   LOG(&consoleData, "Connecting...");
   int status = sysFuncs.connect(sockfd, &sin, 0x10);
   LOG(&consoleData, "Connected.");

   bool hasKernelPermissions = false;
   bool running = true;

   if (sysFuncs.OSEffectiveToPhysical((void *)0xA0000000) != (void *)0x31000000) {
      LOG(&consoleData, "Running without kernel permissions");
      hasKernelPermissions = false;
   } else {
      hasKernelPermissions = true;
   }

   if (status) {
      sockfd = 0;
      LOG(&consoleData, "Error connecting to server, status = %d", status);
   } else {
      while (running) {
         // Read packet header
         if (recvwait(&sysFuncs, sockfd, packetBuffer, 4) <= 0) {
            LOG(&consoleData, "Error reading packet header");
            break;
         }

         // Read packet data
         uint16_t packetSize = ((uint16_t*)packetBuffer)[0];
         uint16_t packetCmd = ((uint16_t*)packetBuffer)[1];
         uint16_t remaining = packetSize - 4;

         if (remaining) {
            if (recvwait(&sysFuncs, sockfd, packetBuffer + 4, remaining) <= 0) {
               LOG(&consoleData, "Error reading packet data");
               break;
            }
         }

         // Process packet
         switch(packetCmd) {
         case 1: {
            // Version
            struct VersionPacket *packet = (struct VersionPacket*)packetBuffer;
            LOG(&consoleData, "Received server version %d", packet->version);

            // Reply with client version
            packet->version = CLIENT_VERSION;
            if (sendwait(&sysFuncs, sockfd, packet, packet->header.size) <= 0) {
               LOG(&consoleData, "Error sending version packet");
               running = false;
            }
         } break;
         case 10: {
            struct ExecuteCodeTestPacket *packet = (struct ExecuteCodeTestPacket*)packetBuffer;

            if (hasKernelPermissions) {
               writeInstruction(&sysFuncs, sysFuncs.PPCMtpmc4, packet->instr);
               executeCodeTest(&packet->state, sysFuncs.PPCMtpmc4);
            } else {
               LOG(&consoleData, "Skipping code test %08X (requires kernel permissions)", packet->instr);
            }

            // Reply with test results
            if (sendwait(&sysFuncs, sockfd, packet, packet->header.size) <= 0) {
               LOG(&consoleData, "Error sending test results.");
               running = false;
            }
         } break;
         case 11: {
            LOG(&consoleData, "Tests finished");
            running = false;
         } break;
         }
      }
   }

renderLoop:
   while(1)
   {
      sysFuncs.VPADRead(0, &vpad_data, 1, &error);

      // Exit when HOME button pressed
      if(vpad_data.btn_hold & BUTTON_HOME) {
         break;
      }

      renderConsole(&consoleData);
   }

   // Safely exit
   {
      unsigned i;
      sysFuncs.socket_lib_finish();
      freeConsole(&sysFuncs, &consoleData);
      sysFuncs.OSFreeToSystem(packetBuffer);

      for(i = 0; i < 2; i++)
      {
         fillScreen(0,0,0,0);
         flipBuffers();
      }

      sysFuncs._Exit();
   }
}

int recvwait(struct SystemFunctions *sysFuncs, int sock, void *buffer, int len)
{
   int ret;
   int recvd = 0;

   while (len > 0) {
      ret = sysFuncs->recv(sock, buffer, len, 0);

      if (ret <= 0) {
         return ret;
      }

      recvd += ret;
      len -= ret;
      buffer += ret;
   }

   return recvd;
}

int sendwait(struct SystemFunctions *sysFuncs, int sock, const void *buffer, int len)
{
   int ret;
   int recvd = 0;

   while (len > 0) {
      ret = sysFuncs->send(sock, buffer, len, 0);

      if (ret <= 0) {
         return ret;
      }

      len -= ret;
      buffer += ret;
   }

   return recvd;
}
