#pragma once
#include "libdecaf/decaf_graphics.h"
#include <SDL.h>

class DecafSDLGraphics
{
public:
   virtual ~DecafSDLGraphics();

   virtual bool
   initialise(int width, int height) = 0;

   virtual void
   shutdown() = 0;

   virtual void
   renderFrame(float tv[4], float drc[4]) = 0;

   virtual decaf::GraphicsDriver *
   getDecafDriver() = 0;

   virtual SDL_Window *
   getWindow();

protected:
   SDL_Window *mWindow = nullptr;

};
