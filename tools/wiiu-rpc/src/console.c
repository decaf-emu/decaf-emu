#include "console.h"

#include <coreinit/cache.h>
#include <coreinit/memheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/screen.h>

#include <string.h>
#include <whb/log.h>

#define NUM_LINES (16)
#define LINE_LENGTH (128)
#define FRAME_HEAP_TAG (0x000DECAF)

static char sConsoleBuffer[NUM_LINES][LINE_LENGTH];
static int sLineNum = 0;
static void *sBufferTV, *sBufferDRC;
static uint32_t sBufferSizeTV, sBufferSizeDRC;

BOOL
consoleInit()
{
   MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
   MEMRecordStateForFrmHeap(heap, FRAME_HEAP_TAG);

   OSScreenInit();
   sBufferSizeTV = OSScreenGetBufferSizeEx(SCREEN_TV);
   sBufferSizeDRC = OSScreenGetBufferSizeEx(SCREEN_DRC);

   sBufferTV = MEMAllocFromFrmHeapEx(heap, sBufferSizeTV, 4);
   if (!sBufferTV) {
      WHBLogPrintf("sBufferTV = MEMAllocFromFrmHeapEx(heap, 0x%X, 4) returned NULL", sBufferSizeTV);
      return FALSE;
   }

   sBufferDRC = MEMAllocFromFrmHeapEx(heap, sBufferSizeDRC, 4);
   if (!sBufferDRC) {
      WHBLogPrintf("sBufferDRC = MEMAllocFromFrmHeapEx(heap, 0x%X, 4) returned NULL", sBufferSizeDRC);
      return FALSE;
   }

   OSScreenSetBufferEx(SCREEN_TV, sBufferTV);
   OSScreenSetBufferEx(SCREEN_DRC, sBufferDRC);

   OSScreenEnableEx(SCREEN_TV, 1);
   OSScreenEnableEx(SCREEN_DRC, 1);
   WHBAddLogHandler(consoleAddLine);
   return TRUE;
}

void
consoleFree()
{
   MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
   OSScreenShutdown();
   MEMFreeByStateToFrmHeap(heap, FRAME_HEAP_TAG);
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

   DCFlushRange(sBufferTV, sBufferSizeTV);
   DCFlushRange(sBufferDRC, sBufferSizeDRC);
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
