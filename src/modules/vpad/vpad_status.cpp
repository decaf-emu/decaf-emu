#include "vpad.h"
#include "vpad_status.h"
#include <Windows.h>
#include <vector>
#include <utility>

static BYTE
gLastVkeyState[256];

static const std::vector<std::pair<Buttons::Buttons, BYTE>>
gControllerMap =
{
   { Buttons::Sync, '0' },
   { Buttons::Home, '1' },
   { Buttons::Minus, '2' },
   { Buttons::Plus, '3' },
   { Buttons::R, 'E' },
   { Buttons::L, 'W' },
   { Buttons::ZR, 'R' },
   { Buttons::ZL, 'Q' },
   { Buttons::Down, VK_DOWN },
   { Buttons::Up, VK_UP },
   { Buttons::Left, VK_LEFT },
   { Buttons::Right, VK_RIGHT },
   { Buttons::Y, 'A' },
   { Buttons::X, 'S' },
   { Buttons::B, 'Z' },
   { Buttons::A, 'X' },
};

int32_t
VPADRead(uint32_t chan, VPADStatus *buffers, uint32_t count, be_val<int32_t> *error)
{
   assert(count >= 1);

   if (error) {
      *error = 0;
   }

   memset(&buffers[0], 0, sizeof(VPADStatus));

   BYTE keyState[256];

   // Flush GetKeyboardState
   GetKeyState(0);

   if (!GetKeyboardState(keyState)) {
      gLog->warn("Could not get keyboard state");
   }

   for (auto &pair : gControllerMap) {
      auto button = pair.first;
      auto vkey = pair.second;

      if (keyState[vkey]) {
         if (!gLastVkeyState[vkey]) {
            buffers[0].trigger |= button;
         }

         buffers[0].hold |= button;
      } else if (gLastVkeyState[vkey]) {
         buffers[0].release |= button;
      }
   }

   memcpy(gLastVkeyState, keyState, sizeof(BYTE) * 256);
   return 1;
}

void
VPad::registerStatusFunctions()
{
   memset(&gLastVkeyState, 0, sizeof(BYTE) * 256);
   RegisterKernelFunction(VPADRead);
}
