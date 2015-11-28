#pragma once
#include <string>

namespace platform
{

namespace ui
{

bool
createWindow(const std::wstring &title);

void
run();

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
