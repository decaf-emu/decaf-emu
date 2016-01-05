#pragma once
#ifdef DECAF_SDL
#include <array>
#include <SDL.h>
#include "platform_interface.h"

namespace platform
{

class PlatformSDL : public Platform
{
   struct JoystickData
   {
      SDL_Joystick *handle = nullptr;
   };

   struct KeyboardData
   {
      const uint8_t *state = nullptr;
      int length = 0;
   };

public:
   virtual ~PlatformSDL() = default;

   virtual bool
   init() override;

   virtual bool
   createWindows(const std::string &tvTitle, const std::string &drcTitle) override;

   virtual void
   run() override;

   virtual void
   shutdown() override;

   virtual void
   activateContext() override;

   virtual void
   swapBuffers() override;

   virtual void
   bindDrcWindow() override;

   virtual int
   getDrcWidth() override;

   virtual int
   getDrcHeight() override;

   virtual void
   bindTvWindow() override;

   virtual int
   getTvWidth() override;

   virtual int
   getTvHeight() override;

   virtual ControllerHandle
   getControllerHandle(const std::string &name) override;

   virtual void
   sampleController(ControllerHandle controller) override;

   virtual ::input::ButtonStatus
   getButtonStatus(ControllerHandle controller, int key) override;

   virtual float
   getAxisValue(ControllerHandle controller, int axis) override;

   virtual int
   getPressedButton(ControllerHandle controller) override;

protected:
   void
   handleEvent(const SDL_Event *event);

   int
   getWindowBorderHeight(SDL_Window *window);

private:
   bool mShouldQuit = false;
   SDL_Window *mTvWindow = nullptr;
   SDL_Window *mDrcWindow = nullptr;
   SDL_GLContext mContext = nullptr;

   ControllerHandle mKeyboardHandle = 1;
   ControllerHandle mJoystickHandleStart = 10;
   ControllerHandle mJoystickHandleEnd = 10 + 16;

   std::array<JoystickData, 16> mJoystickData;
   KeyboardData mKeyboardData;
};

} // namespace platform

#endif // ifdef DECAF_SDL
