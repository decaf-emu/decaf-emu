#pragma once
#include <cstdint>
#include <string>

namespace debugger
{

using ClipboardTextGetCallback = const char *(*)(void *);
using ClipboardTextSetCallback = void(*)(void *, const char*);

void
initialise(const std::string &config,
           ClipboardTextGetCallback getClipboardFn,
           ClipboardTextSetCallback setClipboardFn);

void
shutdown();

void
handleDebugBreakInterrupt();

void
notifyEntry(uint32_t preinit,
            uint32_t entryPoint);

void
draw(unsigned width, unsigned height);

} // namespace debugger
