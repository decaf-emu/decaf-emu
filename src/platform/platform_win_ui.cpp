#include "platform_ui.h"
#include "gpu/driver.h"

#ifdef PLATFORM_WINDOWS
#include <algorithm>
#include <cassert>
#include <Windows.h>

namespace platform
{

namespace ui
{

static HWND gHandle;

static LRESULT WINAPI
WndProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
   LRESULT result = 1;

   switch (msg) {
   case WM_CREATE:
      gHandle = handle;
      gpu::driver::setupWindow();
      break;
   case WM_CLOSE:
      //DestroyWindow(hWnd);
      break;
   case WM_DESTROY:
      PostQuitMessage(0);
      break;
   default:
      result = DefWindowProc(handle, msg, wParam, lParam);
      break;
   }

   return result;
}

bool
createWindow(const std::wstring &title)
{
   static const TCHAR WindowClassName[] = TEXT("WiiUEmuClass");
   HINSTANCE hInstance = GetModuleHandle(NULL);
   WNDCLASS wndclass;

   // Register the frame class
   wndclass.style = 0;
   wndclass.lpfnWndProc = WndProc;
   wndclass.cbClsExtra = 0;
   wndclass.cbWndExtra = 0;
   wndclass.hInstance = hInstance;
   wndclass.hIcon = LoadIcon(hInstance, WindowClassName);
   wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
   wndclass.lpszMenuName = WindowClassName;
   wndclass.lpszClassName = WindowClassName;

   if (!RegisterClass(&wndclass)) {
      return false;
   }

   RECT windowRect = { 0, 0, static_cast<LONG>(getWindowWidth()), static_cast<LONG>(getWindowHeight()) };
   AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

   // Create the frame
   gHandle = CreateWindow(WindowClassName,
                          title.c_str(),
                          WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          windowRect.right - windowRect.left,
                          windowRect.bottom - windowRect.top,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

   if (!gHandle) {
      return false;
   }

   // Show and update main window
   ShowWindow(gHandle, 1);
   UpdateWindow(gHandle);
   return true;
}

void run()
{
   MSG msg;

   while (true) {
      while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == TRUE) {
         if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         } else {
            return;
         }
      }
   }
}

uint64_t
getWindowHandle()
{
   return reinterpret_cast<uint64_t>(gHandle);
}

int
getWindowWidth()
{
   return std::max(getTvWidth(), getDrcWidth());
}

int
getWindowHeight()
{
   return getTvHeight() + getDrcHeight();
}

static const auto DrcScaleFactor = 0.5f;
static const auto TvScaleFactor = 0.65f;

int
getDrcWidth()
{
   return static_cast<int>(854.0f * DrcScaleFactor);
}

int
getDrcHeight()
{
   return static_cast<int>(480.0f * DrcScaleFactor);
}

int
getTvWidth()
{
   return static_cast<int>(1280.0f * TvScaleFactor);
}

int
getTvHeight()
{
   return static_cast<int>(720.0f * TvScaleFactor);
}

}

}

#endif
