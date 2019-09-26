#pragma once
#include <SDL.h>
#include <libgpu/gpu_graphicsdriver.h>

#include <string>
#include <thread>

class SDLWindow
{
   static const auto WindowWidth = 1420;
   static const auto WindowHeight = 768;

public:
   ~SDLWindow();

   bool initCore();
   bool initGraphics();

   bool run(const std::string &tracePath);

private:
   SDL_Window *mWindow = nullptr;
   gpu::GraphicsDriver *mGraphicsDriver = nullptr;
   std::thread mGraphicsThread;
   std::string mRendererName;
   bool mToggleDRC = false;
};
