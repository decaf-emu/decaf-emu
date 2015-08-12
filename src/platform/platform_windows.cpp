#include "../platform.h"
#ifdef PLATFORM_WINDOWS

#include <algorithm>
#include <assert.h>
#include <windows.h>

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

namespace platform {

tm localtime(const std::time_t& time)
{
   std::tm tm_snapshot;
   localtime_s(&tm_snapshot, &time);
   return tm_snapshot;
}

void set_thread_name(std::thread* thread, const std::string& threadName)
{
   DWORD dwThreadID = ::GetThreadId(static_cast<HANDLE>(thread->native_handle()));

   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = threadName.c_str();
   info.dwThreadID = dwThreadID;
   info.dwFlags = 0;

   __try
   {
      RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
   } __except (EXCEPTION_EXECUTE_HANDLER)
   {
   }
}

namespace ui {

TCHAR szAppName[] = TEXT("WiiUEmuClass");
TCHAR szAppTitle[] = TEXT("WiiU Emu");
HWND  ghWnd;

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LRESULT lRet = 1;

   switch (uMsg) {

   case WM_CLOSE:
      //DestroyWindow(hWnd);
      break;

   case WM_DESTROY:
      PostQuitMessage(0);
      break;

   default:
      lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
      break;
   }

   return lRet;
}

void initialise()
{
   WNDCLASS wndclass;
   HINSTANCE hInstance = GetModuleHandle(NULL);

   /* Register the frame class */
   wndclass.style = 0;
   wndclass.lpfnWndProc = WndProc;
   wndclass.cbClsExtra = 0;
   wndclass.cbWndExtra = 0;
   wndclass.hInstance = hInstance;
   wndclass.hIcon = LoadIcon(hInstance, szAppName);
   wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
   wndclass.lpszMenuName = szAppName;
   wndclass.lpszClassName = szAppName;

   if (!RegisterClass(&wndclass))
      return;

   RECT windowRect = { 0, 0, static_cast<LONG>(width()), static_cast<LONG>(height()) };
   AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

   /* Create the frame */
   ghWnd = CreateWindow(szAppName,
      szAppTitle,
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      windowRect.right - windowRect.left,
      windowRect.bottom - windowRect.top,
      NULL,
      NULL,
      hInstance,
      NULL);

   /* show and update main window */
   ShowWindow(ghWnd, 1);

   UpdateWindow(ghWnd);
}

void initialiseCore(int coreId)
{

}

void run()
{
   MSG msg;
   while(true) {
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

uint64_t hwnd()
{
   return (uint64_t)ghWnd;
}

int width() { return std::max(tvWidth(), drcWidth()); }
int height() { return tvHeight() + drcHeight(); }
int tvWidth() { return (int)(1280.0f * 0.65f); }
int tvHeight() { return (int)(720.0f * 0.65f); }
int drcWidth() { return (int)(854.0f * 0.50f); }
int drcHeight() { return (int)(480.0f * 0.50f); }

}

}

#endif
