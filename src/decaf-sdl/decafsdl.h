#pragma once
#include <common-sdl/decafsdl_sound.h>
#include <common-sdl/decafsdl_graphics.h>
#include <libdecaf/decaf.h>
#include <SDL.h>

using namespace decaf::input;

namespace config::input
{

struct InputDevice;

} // namespace config::input

class DecafSDL : public decaf::InputDriver, public decaf::EventListener
{
   static const auto WindowWidth = 1420;
   static const auto WindowHeight = 768;

public:
   ~DecafSDL();

   bool
   initCore();

   bool
   initGlGraphics();

   bool
   initVulkanGraphics();

   bool
   initSound();

   bool
   run(const std::string &gamePath);

private:
   void
   calculateScreenViewports(Viewport &tv, Viewport &drc);

   void
   openInputDevices();

   void sampleVpadController(int channel, vpad::Status &status) override;
   void sampleWpadController(int channel, wpad::Status &status) override;

   // Events
   virtual void
   onGameLoaded(const decaf::GameInfo &info) override;

protected:
   DecafSDLSound *mSoundDriver = nullptr;
   DecafSDLGraphics *mGraphicsDriver = nullptr;

   const config::input::InputDevice *mVpad0Config = nullptr;
   SDL_GameController *mVpad0Controller = nullptr;

   bool mToggleDRC = false;
   bool mGameLoaded = false;
};
