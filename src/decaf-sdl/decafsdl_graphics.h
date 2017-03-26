#pragma once
#include "libdecaf/decaf_graphics.h"
#include <SDL.h>

struct Viewport
{
   float x;
   float y;
   float width;
   float height;
};

class DecafSDLGraphics
{
public:
   virtual ~DecafSDLGraphics();

   virtual bool
   initialise(int width,
              int height) = 0;

   virtual void
   shutdown() = 0;

   virtual void
   renderFrame(Viewport &tv,
               Viewport &drc) = 0;

   virtual decaf::GraphicsDriver *
   getDecafDriver() = 0;

   virtual SDL_Window *
   getWindow();

protected:
   SDL_Window *mWindow		= nullptr;
   SDL_Window *mWindowDRC	= nullptr;
};
