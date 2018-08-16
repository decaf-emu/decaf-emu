#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
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
                    virt_ptr<void> addr);
void
OSScreenPutPixelEx(OSScreenID id,
                   uint32_t x,
                   uint32_t y,
                   uint32_t colour);

void
OSScreenPutFontEx(OSScreenID id,
                  uint32_t row,
                  uint32_t column,
                  virt_ptr<const char> msg);
void
OSScreenFlipBuffersEx(OSScreenID id);

} // namespace cafe::coreinit
