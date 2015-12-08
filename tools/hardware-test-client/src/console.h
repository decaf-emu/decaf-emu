#ifndef CONSOLE_H
#define CONSOLE_H
#include "sysfuncs.h"
#include "../../../libwiiu/src/coreinit.h"
#include "../../../libwiiu/src/types.h"

#define MAX_CONSOLE_LINE_LENGTH 64
#define MAX_CONSOLE_LINES 16
#define LOG(c, ...) __os_snprintf(nextConsoleLine(c), MAX_CONSOLE_LINE_LENGTH, __VA_ARGS__); renderConsole(&consoleData);

struct ConsoleData
{
   unsigned activeLine;
   char *line[MAX_CONSOLE_LINES];
};

void allocConsole(struct SystemFunctions *funcs, struct ConsoleData *console);
void freeConsole(struct SystemFunctions *funcs, struct ConsoleData *console);
void renderConsole(struct ConsoleData *console);
char *nextConsoleLine(struct ConsoleData *console);

#endif
