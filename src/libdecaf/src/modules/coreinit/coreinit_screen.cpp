#include "coreinit.h"
#include "coreinit_screen.h"
#include "coreinit_screen_font.h"
#include "common/decaf_assert.h"
#include "common/log.h"
#include "gpu/gpu_commandqueue.h"
#include "gpu/pm4_packets.h"
#include "gpu/pm4_writer.h"
#include "modules/gx2/gx2_display.h"
#include "modules/gx2/gx2_event.h"
#include "modules/gx2/gx2_state.h"
#include <array>

namespace coreinit
{

struct ScreenSize
{
   uint32_t width;
   uint32_t height;
   uint32_t pitch;
};

static const auto
BytesPerPixel = 4;

static std::array<ScreenSize, OSScreenID::Max>
sScreenSizes =
{
   ScreenSize { 1280, 720, 1280 },
   ScreenSize { 854, 480, 854 }
};

static std::array<uint32_t *, OSScreenID::Max>
sBuffers;

void
OSScreenInit()
{
   gx2::GX2Init(nullptr);

   gx2::GX2SetTVBuffer(nullptr,
                       OSScreenGetBufferSizeEx(OSScreenID::TV),
                       gx2::GX2TVRenderMode::Wide720p,
                       gx2::GX2SurfaceFormat::UNORM_R8_G8_B8_A8,
                       gx2::GX2BufferingMode::Single);

   gx2::GX2SetDRCBuffer(nullptr,
                       OSScreenGetBufferSizeEx(OSScreenID::TV),
                       gx2::GX2DrcRenderMode::Single,
                       gx2::GX2SurfaceFormat::UNORM_R8_G8_B8_A8,
                       gx2::GX2BufferingMode::Single);

   sBuffers.fill(nullptr);
}

uint32_t
OSScreenGetBufferSizeEx(OSScreenID id)
{
   decaf_check(id < OSScreenID::Max);
   auto &size = sScreenSizes[id];
   return size.pitch * size.height * BytesPerPixel;
}

void
OSScreenEnableEx(OSScreenID id,
                 BOOL enable)
{
}

void
OSScreenClearBufferEx(OSScreenID id,
                      uint32_t colour)
{
   decaf_check(id < OSScreenID::Max);
   auto size = OSScreenGetBufferSizeEx(id) / 4;
   auto buffer = sBuffers[id];

   // Force alpha to 255
   colour = byte_swap(colour) | 0xff000000;

   for (auto i = 0u; i < size; ++i) {
      buffer[i] = colour;
   }
}

void
OSScreenSetBufferEx(OSScreenID id,
                    void *addr)
{
   decaf_check(id < OSScreenID::Max);
   sBuffers[id] = reinterpret_cast<uint32_t *>(addr);
}

void
OSScreenPutPixelEx(OSScreenID id,
                   uint32_t x,
                   uint32_t y,
                   uint32_t colour)
{
   decaf_check(id < OSScreenID::Max);
   auto buffer = sBuffers[id];
   auto size = sScreenSizes[id];

   // Force alpha to 255
   colour = byte_swap(colour) | 0xff000000;

   if (buffer && x < size.width && y < size.height) {
      auto offset = x + y * size.pitch;
      buffer[offset] = colour;
   }
}

static void
putChar(OSScreenID id,
        uint32_t x,
        uint32_t y,
        char chr)
{
   auto index = chr & 0x7F;

   if (index < ' ') {
      index = 0;
   } else {
      index -= ' ';
   }

   auto font = sScreenFontBitmap + index * sScreenFontPitch;

   for (auto v = 0; v < sScreenFontHeight; ++v) {
      for (auto h = 0; h < sScreenFontWidth; ++h) {
         auto bitmap = font[v * 2 + h / 8];
         auto bit = bitmap >> (h % 8);

         if (bit & 1) {
            OSScreenPutPixelEx(id, x + h, y + v, 0xFFFFFFFF);
         }
      }
   }
}

void
OSScreenPutFontEx(OSScreenID id,
                  uint32_t row,
                  uint32_t column,
                  const char *msg)
{
   static const auto offsetX = 50;
   static const auto offsetY = 32;
   static const auto adjustX = 12;
   static const auto adjustY = 24;
   auto x = offsetX + row * adjustX;
   auto y = offsetY + column * adjustY;

   while (msg && *msg) {
      putChar(id, x, y, *msg);
      x += adjustX;
      msg++;
   }
}

void
OSScreenFlipBuffersEx(OSScreenID id)
{
   decaf_check(id < OSScreenID::Max);
   auto buffer = sBuffers[id];
   auto size = OSScreenGetBufferSizeEx(id);

   // Send the custom flip command
   pm4::write(pm4::DecafOSScreenFlip {
      static_cast<uint32_t>(id),
      buffer
   });

   // Wait until flip
   gx2::GX2Flush();
   gx2::GX2WaitForFlip();
}

void
Module::registerScreenFunctions()
{
   RegisterKernelFunction(OSScreenInit);
   RegisterKernelFunction(OSScreenGetBufferSizeEx);
   RegisterKernelFunction(OSScreenEnableEx);
   RegisterKernelFunction(OSScreenClearBufferEx);
   RegisterKernelFunction(OSScreenSetBufferEx);
   RegisterKernelFunction(OSScreenPutPixelEx);
   RegisterKernelFunction(OSScreenPutFontEx);
   RegisterKernelFunction(OSScreenFlipBuffersEx);
}

} // namespace coreinit
