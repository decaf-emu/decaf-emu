#include <common/platform.h>

#ifdef PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <SDL.h>
#include <SDL_syswm.h>

extern "C"
{
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

void
setWindowIcon(SDL_Window *window)
{
   auto handle = ::GetModuleHandle(nullptr);
   auto icon = ::LoadIcon(handle, L"IDI_MAIN_ICON");

   if (icon != nullptr) {
      SDL_SysWMinfo wminfo;
      SDL_VERSION(&wminfo.version);

      if (SDL_GetWindowWMInfo(window, &wminfo) == 1) {
         auto hwnd = wminfo.info.win.window;
         ::SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon));
      }
   }
}

#endif
