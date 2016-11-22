#pragma once
#include "common/types.h"

namespace coreinit
{

enum OSScreenID : uint32_t
{
   TV = 0,
   DRC = 1,
   Max
};

void
OSScreenInit();

uint32_t
OSScreenGetBufferSizeEx(OSScreenID id);

void
OSScreenEnableEx(OSScreenID id,
                 BOOL enable);
void
OSScreenClearBufferEx(OSScreenID id,
                      uint32_t colour);
void
OSScreenSetBufferEx(OSScreenID id,
                    void *addr);
void
OSScreenPutPixelEx(OSScreenID id,
                   uint32_t x,
                   uint32_t y,
                   uint32_t colour);

void
OSScreenPutFontEx(OSScreenID id,
                  uint32_t row,
                  uint32_t column,
                  const char *msg);
void
OSScreenFlipBuffersEx(OSScreenID id);

} // namespace coreinit
