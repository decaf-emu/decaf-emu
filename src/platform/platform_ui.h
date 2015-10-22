#pragma once
#include <string>
#include "platform/platform.h"

namespace platform
{

namespace ui
{

bool
createWindow(const std::wstring &title);

void
run();

uint64_t
getWindowHandle();

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
