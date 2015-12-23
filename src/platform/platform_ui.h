#pragma once
#include <string>

namespace platform
{

namespace ui
{

bool
init();

bool
createWindows(const std::string &tvTitle, const std::string &drcTitle);

void
run();

void
shutdown();

void
activateContext();

void
swapBuffers();

void
bindDrcWindow();

int
getDrcWidth();

int
getDrcHeight();

void
bindTvWindow();

int
getTvWidth();

int
getTvHeight();

} // namespace ui

} // namespace platform
