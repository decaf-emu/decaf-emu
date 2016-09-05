#include "decafsdl_graphics.h"

DecafSDLGraphics::~DecafSDLGraphics()
{
   if (mWindow) {
      SDL_DestroyWindow(mWindow);
      mWindow = nullptr;
   }
}

SDL_Window *
DecafSDLGraphics::getWindow()
{
   return mWindow;
}