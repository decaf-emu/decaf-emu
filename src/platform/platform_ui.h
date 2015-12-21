#pragma once
#include <string>

namespace platform
{

namespace ui
{

bool
init();

bool
createWindow(const std::string &title);

void
run();

void
shutdown();

void
swapBuffers();

void
activateContext();

void
releaseContext();

int
getWindowWidth();

int
getWindowHeight();

int
getDrcWidth();

int
getDrcHeight();

int
getTvWidth();

int
getTvHeight();

} // namespace ui

} // namespace platform
