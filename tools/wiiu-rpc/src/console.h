#ifndef CONSOLE_H
#define CONSOLE_H

#include <coreinit/internal.h>

BOOL
consoleInit();

void
consoleFree();

void
consoleDraw();

void
consoleAddLine(const char *buffer);

#endif // CONSOLE_H
