#pragma once
#include <string>
#include "types.h"
#include "input/input.h"
#include "platform_input.h"

namespace platform
{

class Platform
{
public:
   virtual ~Platform() = default;

   virtual bool
   init() = 0;

   virtual bool
   createWindows(const std::string &tvTitle, const std::string &drcTitle) = 0;

   virtual void
   setTvTitle(const std::string &title) = 0;

   virtual void
   setDrcTitle(const std::string &title) = 0;

   virtual void
   run() = 0;

   virtual void
   shutdown() = 0;

   virtual void
   activateContext() = 0;

   virtual void
   swapBuffers() = 0;

   virtual void
   bindDrcWindow() = 0;

   virtual int
   getDrcWidth() = 0;

   virtual int
   getDrcHeight() = 0;

   virtual void
   bindTvWindow() = 0;

   virtual int
   getTvWidth() = 0;

   virtual int
   getTvHeight() = 0;

   virtual ControllerHandle
   getControllerHandle(const std::string &name) = 0;

   virtual void
   sampleController(ControllerHandle controller) = 0;

   virtual ::input::ButtonStatus
   getButtonStatus(ControllerHandle controller, int key) = 0;

   virtual float
   getAxisValue(ControllerHandle controller, int axis) = 0;

   virtual int
   getPressedButton(ControllerHandle controller) = 0;
};

void
setPlatform(Platform *platform);

} // namespace platform
