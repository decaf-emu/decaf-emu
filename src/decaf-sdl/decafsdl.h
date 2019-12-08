#pragma once
#include "decafsdl_sound.h"

#include <libdecaf/decaf.h>
#include <SDL.h>
#include <thread>

using namespace decaf::input;

namespace gpu { class GraphicsDriver; }

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

   bool initCore();
   bool initGraphics();
   bool initSound();

   bool run(const std::string &gamePath);

private:
   void openInputDevices();

   void sampleVpadController(int channel, vpad::Status &status) override;
   void sampleWpadController(int channel, wpad::Status &status) override;

   // Events
   virtual void onGameLoaded(const decaf::GameInfo &info) override;

protected:
   DecafSDLSound *mSoundDriver = nullptr;

   SDL_Window *mWindow = nullptr;
   gpu::GraphicsDriver *mGraphicsDriver = nullptr;
   std::thread mGraphicsThread;

   const config::input::InputDevice *mVpad0Config = nullptr;
   SDL_GameController *mVpad0Controller = nullptr;

   bool mToggleDRC = false;

   Uint32 mDecafEventId = -1;
   Uint32 mUpdateWindowTitleEventId = -1;
   SDL_TimerID mWindowTitleTimerId = -1;

   decaf::GameInfo mGameInfo;
};
