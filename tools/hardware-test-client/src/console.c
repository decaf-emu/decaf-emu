#include "console.h"

void allocConsole(struct SystemFunctions *funcs, struct ConsoleData *console)
{
   unsigned i;
   console->activeLine = 0;

   for (i = 0; i < MAX_CONSOLE_LINES; ++i) {
      console->line[i] = funcs->OSAllocFromSystem(MAX_CONSOLE_LINE_LENGTH, 4);
      console->line[i][0] = 0;
   }
}

void freeConsole(struct SystemFunctions *funcs, struct ConsoleData *console)
{
   unsigned i;

   for (i = 0; i < MAX_CONSOLE_LINES; ++i) {
      funcs->OSFreeToSystem(console->line[i]);
   }
}

void renderConsole(struct ConsoleData *console)
{
   unsigned i, j;

   for(i = 0; i < 2; i++)
   {
      fillScreen(0, 0, 0, 0);

      for (j = 0; j < MAX_CONSOLE_LINES; ++j) {
         drawString(0, j, console->line[j]);
      }

      flipBuffers();
   }
}

char *nextConsoleLine(struct ConsoleData *console)
{
   if (console->activeLine < MAX_CONSOLE_LINES) {
      return console->line[console->activeLine++];
   } else {
      char *nextLine = console->line[0];
      unsigned i;

      // Shift all lines up screen 1
      for (i = 0; i < MAX_CONSOLE_LINES - 1; ++i) {
         console->line[i] = console->line[i + 1];
      }

      // Move 0th line to end of console, reuse for nextLine
      console->line[MAX_CONSOLE_LINES - 1] = nextLine;

      // Clear line and return it
      nextLine[0] = 0;
      return nextLine;
   }
}
