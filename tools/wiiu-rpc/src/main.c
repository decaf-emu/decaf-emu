#include "console.h"
#include "server.h"
#include "packet.h"

#include <coreinit/core.h>
#include <coreinit/dynload.h>
#include <coreinit/filesystem.h>
#include <coreinit/foreground.h>
#include <coreinit/ios.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memheap.h>
#include <coreinit/screen.h>
#include <coreinit/systeminfo.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <nn/ac/ac_c.h>
#include <nsysnet/socket.h>
#include <sysapp/launch.h>
#include <whb/crash.h>
#include <whb/file.h>
#include <whb/gfx.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/proc.h>

#include <string.h>
#include <malloc.h>

typedef enum PacketCommand
{
   CMD_DYNLOAD_ACQUIRE = 1000,
   CMD_DYNLOAD_RELEASE = 1001,
   CMD_DYNLOAD_FINDEXPORT = 1002,
   CMD_READ_MEMORY = 2000,
   CMD_WRITE_MEMORY = 2001,
   CMD_CALL_FUNCTION = 2002,
   CMD_IOS_OPEN = 3000,
   CMD_IOS_CLOSE = 3001,
   CMD_IOS_IOCTL = 3002,
   CMD_IOS_IOCTLV = 3003,
} PacketCommand;

typedef enum ArgTypes
{
   ARG_TYPE_INT32 = 0,
   ARG_TYPE_INT64 = 1,
   ARG_TYPE_STRING = 2,
   ARG_TYPE_DATA_IN = 3,
   ARG_TYPE_DATA_OUT = 4,
   ARG_TYPE_DATA_IN_OUT = 5,
} ArgTypes;

#define MAX_NUM_ARGS 5

static uint32_t
callFuncArgs0(void *func)
{
   WHBLogPrintf("%p()", func);
   return ((uint32_t (*)())func)();
}

static uint32_t
callFuncArgs1(void *func, uint32_t *args)
{
   WHBLogPrintf("%p(%x)", func, args[0]);
   return ((uint32_t (*)(uint32_t))func)(args[0]);
}

static uint32_t
callFuncArgs2(void *func, uint32_t *args)
{
   WHBLogPrintf("%p(%x, %x)", func, args[0], args[1]);
   return ((uint32_t (*)(uint32_t, uint32_t))func)(args[0], args[1]);
}

static uint32_t
callFuncArgs3(void *func, uint32_t *args)
{
   WHBLogPrintf("%p(%x, %x, %x)", func, args[0], args[1], args[2]);
   return ((uint32_t (*)(uint32_t, uint32_t, uint32_t))func)(args[0], args[1], args[2]);
}

static uint32_t
callFuncArgs4(void *func, uint32_t *args)
{
   WHBLogPrintf("%p(%x, %x, %x, %x)", func, args[0], args[1], args[2], args[3]);
   return ((uint32_t (*)(uint32_t, uint32_t, uint32_t, uint32_t))func)(args[0], args[1], args[2], args[3]);
}

static uint32_t
callFuncArgs5(void *func, uint32_t *args)
{
   WHBLogPrintf("%p(%x, %x, %x, %x, %x)", func, args[0], args[1], args[2], args[3], args[4]);
   return ((uint32_t (*)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t))func)(args[0], args[1], args[2], args[3], args[4]);
}

int
packetHandler(Server *server, PacketReader *packet)
{
   PacketWriter out;
   MEMHeapHandle mem2 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);

   switch (packet->command) {
   case CMD_DYNLOAD_ACQUIRE:
   {
      const char *name = pakReadString(packet);
      OSDynLoad_Module module = NULL;
      int result;

      WHBLogPrintf("OSDynLoad_Acquire(\"%s\")", name);
      result = OSDynLoad_Acquire(name, &module);

      pakWriteAlloc(&out, CMD_DYNLOAD_ACQUIRE);
      pakWriteInt32(&out, result);
      pakWritePointer(&out, module);
      serverSendPacket(server, &out);
      pakWriteFree(&out);
      break;
   }
   case CMD_DYNLOAD_RELEASE:
   {
      OSDynLoad_Module *module = (OSDynLoad_Module *)pakReadPointer(packet);

      WHBLogPrintf("OSDynLoad_Release(%p)", module);
      OSDynLoad_Release(module);

      pakWriteAlloc(&out, CMD_DYNLOAD_RELEASE);
      serverSendPacket(server, &out);
      pakWriteFree(&out);
      break;
   }
   case CMD_DYNLOAD_FINDEXPORT:
   {
      OSDynLoad_Module *module = (OSDynLoad_Module *)pakReadPointer(packet);
      int32_t isData = pakReadInt32(packet);
      const char *name = pakReadString(packet);
      void *addr = NULL;
      int result = 0;

      WHBLogPrintf("OSDynLoad_FindExport(%p, %d, \"%s\")", module, isData, name);
      result = OSDynLoad_FindExport(module, isData, name, &addr);

      pakWriteAlloc(&out, CMD_DYNLOAD_FINDEXPORT);
      pakWriteInt32(&out, result);
      pakWritePointer(&out, addr);
      serverSendPacket(server, &out);
      pakWriteFree(&out);
      break;
   }
   case CMD_READ_MEMORY:
   {
      void *src = pakReadPointer(packet);
      uint32_t size = pakReadUint32(packet);

      pakWriteAlloc(&out, CMD_DYNLOAD_FINDEXPORT);
      pakWriteData(&out, src, size);
      serverSendPacket(server, &out);
      pakWriteFree(&out);
      break;
   }
   case CMD_WRITE_MEMORY:
   {
      void *dst = pakReadPointer(packet);
      uint32_t size;
      const uint8_t *data = pakReadData(packet, &size);

      memcpy(dst, data, size);

      pakWriteAlloc(&out, CMD_WRITE_MEMORY);
      serverSendPacket(server, &out);
      pakWriteFree(&out);
      break;
   }
   case CMD_CALL_FUNCTION:
   {
      void *func = pakReadPointer(packet);
      uint32_t numArgs = pakReadUint32(packet);
      uint32_t argsArray[MAX_NUM_ARGS * 2] = { 0 };
      uint8_t *argsTmpData[MAX_NUM_ARGS] = { 0 };
      uint32_t argsTmpDataSize[MAX_NUM_ARGS] = { 0 };
      uint32_t numCallArgs = 0;
      uint32_t numTmpData = 0;
      uint32_t result = 0;

      WHBLogPrintf("Call function %p", func);

      for (int i = 0; i < numArgs; i++) {
         uint32_t argType = pakReadUint32(packet);

         if (argType == ARG_TYPE_INT32) {
            argsArray[numCallArgs++] = pakReadUint32(packet);
         } else if (argType == ARG_TYPE_INT64) {
            uint64_t value = pakReadUint64(packet);
            argsArray[numCallArgs++] = (uint32_t)((value >> 32) & 0xFFFFFFFF);
            argsArray[numCallArgs++] = (uint32_t)(value & 0xFFFFFFFF);
         } else if (argType == ARG_TYPE_STRING) {
            const char *str = pakReadString(packet);
            argsArray[numCallArgs++] = (uint32_t)str;
         } else if (argType == ARG_TYPE_DATA_IN) {
            uint32_t size;
            const uint8_t *data = pakReadData(packet, &size);
            argsArray[numCallArgs++] = (uint32_t)data;
         } else if (argType == ARG_TYPE_DATA_OUT) {
            argsTmpDataSize[i] = pakReadUint32(packet);
            argsTmpData[i] = MEMAllocFromExpHeapEx(mem2, argsTmpDataSize[i], 0x100);
            argsArray[numCallArgs++] = (uint32_t)argsTmpData[i];
            numTmpData++;
         } else if (argType == ARG_TYPE_DATA_IN_OUT) {
            uint32_t size;
            const uint8_t *data = pakReadData(packet, &size);
            argsTmpDataSize[i] = size;
            argsTmpData[i] = MEMAllocFromExpHeapEx(mem2, argsTmpDataSize[i], 0x100);
            memcpy(argsTmpData[i], data, argsTmpDataSize[i]);
            argsArray[numCallArgs++] = (uint32_t)argsTmpData[i];
            numTmpData++;
         }
      }

      if (numCallArgs == 0) {
         result = callFuncArgs0(func);
      } else if (numCallArgs == 1) {
         result = callFuncArgs1(func, argsArray);
      } else if (numCallArgs == 2) {
         result = callFuncArgs2(func, argsArray);
      } else if (numCallArgs == 3) {
         result = callFuncArgs3(func, argsArray);
      } else if (numCallArgs == 4) {
         result = callFuncArgs4(func, argsArray);
      } else if (numCallArgs == 5) {
         result = callFuncArgs5(func, argsArray);
      }

      pakWriteAlloc(&out, CMD_CALL_FUNCTION);
      pakWriteInt32(&out, result);
      pakWriteUint32(&out, numTmpData);

      for (int i = 0; i < numArgs; ++i) {
         if (argsTmpData[i]) {
            pakWriteUint32(&out, i);
            pakWriteData(&out, argsTmpData[i], argsTmpDataSize[i]);
            MEMFreeToExpHeap(mem2, argsTmpData[i]);
         }
      }

      serverSendPacket(server, &out);
      pakWriteFree(&out);
      break;
   }
   case CMD_IOS_IOCTLV:
   {
      IOSHandle handle = (IOSHandle)pakReadUint32(packet);
      uint32_t request = pakReadUint32(packet);
      uint32_t vecIn = pakReadUint32(packet);
      uint32_t vecOut = pakReadUint32(packet);
      IOSVec *vecs = MEMAllocFromExpHeapEx(mem2, sizeof(IOSVec) * (vecIn + vecOut), 0x100);
      void **vecsPtrs = malloc(sizeof(void *) * (vecIn + vecOut));
      uint32_t *vecsLen = malloc(sizeof(uint32_t) * (vecIn + vecOut));
      IOSError result;

      for (int i = 0; i < vecIn; ++i) {
         const uint8_t *data = pakReadData(packet, &vecsLen[i]);
         vecsPtrs[i] = MEMAllocFromExpHeapEx(mem2, vecsLen[i], 0x100);

         vecs[i].len = vecsLen[i];
         vecs[i].paddr = vecsPtrs[i];
         memcpy(vecsPtrs[i], data, vecsLen[i]);
      }

      for (int i = vecIn; i < vecIn + vecOut; ++i) {
         vecsLen[i] = pakReadUint32(packet);
         vecsPtrs[i] = MEMAllocFromExpHeapEx(mem2, vecsLen[i], 0x100);
         vecs[i].len = vecsLen[i];
         vecs[i].paddr = vecsPtrs[i];
         memset(vecsPtrs[i], 0, vecsLen[i]);
      }

      result = IOS_Ioctlv(handle, request, vecIn, vecOut, vecs);

      pakWriteAlloc(&out, CMD_IOS_IOCTLV);
      pakWriteInt32(&out, (int32_t)result);

      for (int i = vecIn; i < vecIn + vecOut; ++i) {
         pakWriteData(&out, vecsPtrs[i], vecsLen[i]);
      }

      serverSendPacket(server, &out);
      pakWriteFree(&out);

      for (int i = 0; i < vecIn + vecOut; ++i) {
         MEMFreeToExpHeap(mem2, vecsPtrs[i]);
      }

      MEMFreeToExpHeap(mem2, vecs);
      free(vecsPtrs);
      free(vecsLen);
      break;
   }
   default:
      WHBLogPrintf("Unknown packet command: %d", packet->command);
      return -1;
   }

   return 0;
}

int
main(int argc, char **argv)
{
   Server server;
   int result = 0;

   WHBProcInit(TRUE);
   WHBLogUdpInit();
   WHBInitCrashHandler();

   if (!consoleInit()) {
      result = -1;
      goto exit;
   }

   socket_lib_init();
   ACInitialize();

   if (serverStart(&server, 1337) == 0) {
      while(WHBProcIsRunning()) {
         consoleDraw();
         serverProcess(&server, &packetHandler);
         OSSleepTicks(OSMillisecondsToTicks(30));
      }
   } else {
      WHBLogPrintf("Failed to start server.");
   }

exit:
   WHBLogPrintf("Exiting...");
   serverClose(&server);
   socket_lib_finish();
   ACFinalize();
   consoleFree();
   WHBProcShutdown();
   return result;
}
