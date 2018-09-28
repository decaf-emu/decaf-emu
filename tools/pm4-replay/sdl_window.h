#pragma once
#include <common-sdl/decafsdl_graphics.h>
#include <string>

class SDLWindow
{
   static const auto WindowWidth = 1420;
   static const auto WindowHeight = 768;

public:
   ~SDLWindow();

   bool
   run(const std::string &tracePath);

   bool
   initCore();

   bool
   initGlGraphics();

   bool
   initVulkanGraphics();

   void
   calculateScreenViewports(Viewport &tv, Viewport &drc);

private:
   DecafSDLGraphics *mRenderer = nullptr;
   std::string mRendererName;
   bool mToggleDRC = false;
};
