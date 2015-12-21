#pragma once

union SDL_Event;
struct SDL_Window;

namespace platform
{

namespace sdl
{

extern bool shouldQuit;

extern SDL_Window *tvWindow;
extern SDL_Window *drcWindow;

void
handleEvent(const SDL_Event *event);

} // namespace sdl

} // namespace platform
