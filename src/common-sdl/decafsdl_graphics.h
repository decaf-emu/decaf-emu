#pragma once
#include <libdecaf/decaf_graphics.h>
#include <libdebugui/debugui.h>
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
              int height,
              bool renderDebugger = true) = 0;

   virtual void
   shutdown() = 0;

   virtual void
   windowResized() = 0;

   virtual void
   renderFrame(Viewport &tv,
               Viewport &drc) = 0;

   virtual gpu::GraphicsDriver *
   getDecafDriver() = 0;

   virtual debugui::Renderer *
   getDebugUiRenderer() = 0;

   virtual SDL_Window *
   getWindow();

   virtual void
   getWindowSize(int *w, int *h);

protected:
   SDL_Window *mWindow = nullptr;
};
