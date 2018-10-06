#pragma once
#include "decafsdl_sound.h"

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

   decaf::input::KeyboardKey
   translateKeyCode(SDL_Keysym sym);

   decaf::input::MouseButton
   translateMouseButton(int button);

   // VPAD
   virtual vpad::Type
   getControllerType(vpad::Channel channel) override;

   virtual ButtonStatus
   getButtonStatus(vpad::Channel channel, vpad::Core button) override;

   virtual float
   getAxisValue(vpad::Channel channel, vpad::CoreAxis axis) override;

   virtual bool
   getTouchPosition(vpad::Channel channel, vpad::TouchPosition &position) override;

   // WPAD
   virtual wpad::Type
   getControllerType(wpad::Channel channel) override;

   virtual ButtonStatus
   getButtonStatus(wpad::Channel channel, wpad::Core button) override;

   virtual ButtonStatus
   getButtonStatus(wpad::Channel channel, wpad::Classic button) override;

   virtual ButtonStatus
   getButtonStatus(wpad::Channel channel, wpad::Nunchuck button) override;

   virtual ButtonStatus
   getButtonStatus(wpad::Channel channel, wpad::Pro button) override;

   virtual float
   getAxisValue(wpad::Channel channel, wpad::NunchuckAxis axis) override;

   virtual float
   getAxisValue(wpad::Channel channel, wpad::ProAxis axis) override;

   // Events
   virtual void
   onGameLoaded(const decaf::GameInfo &info) override;

protected:
   DecafSDLSound *mSoundDriver = nullptr;
   DecafSDLGraphics *mGraphicsDriver = nullptr;

   const config::input::InputDevice *mVpad0Config = nullptr;
   const config::input::InputDevice *mWpadConfig[4] = { nullptr, nullptr, nullptr, nullptr };
   SDL_GameController *mVpad0Controller = nullptr;
   SDL_GameController *mWpadController[4] = { nullptr, nullptr, nullptr, nullptr };

   bool mToggleDRC = false;
   bool mGameLoaded = false;
};
