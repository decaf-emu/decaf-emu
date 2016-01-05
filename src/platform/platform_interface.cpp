#include "platform_interface.h"

namespace platform
{

static Platform *
gPlatform = nullptr;

void
setPlatform(Platform *platform)
{
   gPlatform = platform;
}

namespace ui
{

bool
init()
{
   return gPlatform->init();
}

bool
createWindows(const std::string &tvTitle, const std::string &drcTitle)
{
   return gPlatform->createWindows(tvTitle, drcTitle);
}

void
run()
{
   gPlatform->run();
}

void
shutdown()
{
   gPlatform->shutdown();
}

void
activateContext()
{
   gPlatform->activateContext();
}

void
swapBuffers()
{
   gPlatform->swapBuffers();
}

void
bindDrcWindow()
{
   gPlatform->bindDrcWindow();
}

int
getDrcWidth()
{
   return gPlatform->getDrcWidth();
}

int
getDrcHeight()
{
   return gPlatform->getDrcHeight();
}

void
bindTvWindow()
{
   gPlatform->bindTvWindow();
}

int
getTvWidth()
{
   return gPlatform->getTvWidth();
}

int
getTvHeight()
{
   return gPlatform->getTvHeight();
}

} // namespace ui

namespace input
{

ControllerHandle
getControllerHandle(const std::string &name)
{
   return gPlatform->getControllerHandle(name);
}

void
sampleController(ControllerHandle controller)
{
   gPlatform->sampleController(controller);
}

::input::ButtonStatus
getButtonStatus(ControllerHandle controller, int key)
{
   return gPlatform->getButtonStatus(controller, key);
}

float
getAxisValue(ControllerHandle controller, int axis)
{
   return gPlatform->getAxisValue(controller, axis);
}

} // namespace input

} // namespace platform
