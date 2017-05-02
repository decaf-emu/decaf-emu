#include "console.h"

#include <coreinit/baseheap.h>
#include <coreinit/cache.h>
#include <coreinit/expandedheap.h>
#include <coreinit/frameheap.h>
#include <coreinit/screen.h>

#include <string.h>

#define NUM_LINES (16)
#define LINE_LENGTH (128)

static char sConsoleBuffer[NUM_LINES][LINE_LENGTH];
static int sLineNum = 0;
static void *bufferTV, *bufferDRC;
static uint32_t bufferSizeTV, bufferSizeDRC;

void
consoleInit()
{
   MEMHeapHandle mem1;

   OSScreenInit();
   bufferSizeTV = OSScreenGetBufferSizeEx(SCREEN_TV);
   bufferSizeDRC = OSScreenGetBufferSizeEx(SCREEN_DRC);

   mem1 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);
   bufferTV = MEMAllocFromExpHeapEx(mem1, bufferSizeTV, 4);
   bufferDRC = MEMAllocFromExpHeapEx(mem1, bufferSizeDRC, 4);

   OSScreenSetBufferEx(SCREEN_TV, bufferTV);
   OSScreenSetBufferEx(SCREEN_DRC, bufferDRC);

   OSScreenEnableEx(SCREEN_TV, 1);
   OSScreenEnableEx(SCREEN_DRC, 1);
}

void
consoleFree()
{
   OSScreenShutdown();
   MEMHeapHandle mem1 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2);
   MEMFreeToExpHeap(mem1, bufferTV);
   MEMFreeToExpHeap(mem1, bufferDRC);
}

void
consoleDraw()
{
   OSScreenClearBufferEx(SCREEN_TV, 0x993333FF);
   OSScreenClearBufferEx(SCREEN_DRC, 0x993333FF);

   for (int y = 0; y < NUM_LINES; ++y) {
      OSScreenPutFontEx(SCREEN_TV, 0, y, sConsoleBuffer[y]);
      OSScreenPutFontEx(SCREEN_DRC, 0, y, sConsoleBuffer[y]);
   }

   DCFlushRange(bufferTV, bufferSizeTV);
   DCFlushRange(bufferDRC, bufferSizeDRC);
   OSScreenFlipBuffersEx(SCREEN_TV);
   OSScreenFlipBuffersEx(SCREEN_DRC);
}

void
consoleAddLine(const char *line)
{
   int length = strlen(line);

   if (length > LINE_LENGTH) {
      length = LINE_LENGTH - 1;
   }

   if (sLineNum == NUM_LINES) {
      for (int i = 0; i < NUM_LINES - 1; ++i) {
         memcpy(sConsoleBuffer[i], sConsoleBuffer[i + 1], LINE_LENGTH);
      }

      memcpy(sConsoleBuffer[sLineNum - 1], line, length);
      sConsoleBuffer[sLineNum - 1][length] = 0;
   } else {
      memcpy(sConsoleBuffer[sLineNum], line, length);
      sConsoleBuffer[sLineNum][length] = 0;
      ++sLineNum;
   }
}
