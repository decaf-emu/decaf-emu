#ifndef CONSOLE_H
#define CONSOLE_H

#include <coreinit/internal.h>

void
consoleInit();

void
consoleFree();

void
consoleDraw();

void
consoleAddLine(const char *buffer);

#define consoleLog(fmt, ...) \
   { \
      char printf_buffer[128]; \
      __os_snprintf(printf_buffer, 128, fmt, ##__VA_ARGS__); \
      consoleAddLine(printf_buffer); \
   }

#endif // CONSOLE_H
