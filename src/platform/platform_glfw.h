#pragma once
#ifdef DECAF_GLFW
#include <array>
#include <GLFW/glfw3.h>
#include "platform_interface.h"

namespace platform
{

class PlatformGLFW : public Platform
{
   struct JoystickData
   {
      int count = 0;
      const unsigned char *buttons = nullptr;
   };

public:
   virtual ~PlatformGLFW() = default;

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

   virtual int
   getPressedButton(ControllerHandle controller) override;

protected:
   int
   getWindowWidth();

   int
   getWindowHeight();

private:
   GLFWwindow *mWindow;
   std::array<JoystickData, GLFW_JOYSTICK_LAST> mJoystickData;
   ControllerHandle mKeyboardHandle = 1;
   ControllerHandle mJoystickHandleStart = 10;
   ControllerHandle mJoystickHandleEnd = 10 + GLFW_JOYSTICK_LAST;
};

} // namespace platform

#endif // ifdef DECAF_GLFW
