#include "../platform.h"
#ifdef PLATFORM_WINDOWS

#include <assert.h>
#include <windows.h>
#include <gl/GL.h>

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
HDC   ghDC;
HGLRC ghRC[3];

BOOL bSetupPixelFormat(HDC hdc)
{
   PIXELFORMATDESCRIPTOR pfd, *ppfd;
   int pixelformat;

   ppfd = &pfd;

   ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
   ppfd->nVersion = 1;
   ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
      PFD_DOUBLEBUFFER;
   ppfd->dwLayerMask = PFD_MAIN_PLANE;
   ppfd->iPixelType = PFD_TYPE_COLORINDEX;
   ppfd->cColorBits = 8;
   ppfd->cDepthBits = 16;
   ppfd->cAccumBits = 0;
   ppfd->cStencilBits = 0;

   pixelformat = ChoosePixelFormat(hdc, ppfd);

   if ((pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0)
   {
      MessageBox(NULL, TEXT("ChoosePixelFormat failed"), TEXT("Error"), MB_OK);
      return FALSE;
   }

   if (SetPixelFormat(hdc, pixelformat, ppfd) == FALSE)
   {
      MessageBox(NULL, TEXT("SetPixelFormat failed"), TEXT("Error"), MB_OK);
      return FALSE;
   }

   return TRUE;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LRESULT lRet = 1;
   PAINTSTRUCT ps;

   switch (uMsg) {

   case WM_PAINT:
      BeginPaint(hWnd, &ps);
      EndPaint(hWnd, &ps);
      break;

   case WM_CLOSE:
      /*
      if (ghDC)
         ReleaseDC(hWnd, ghDC);
      ghDC = 0;

      DestroyWindow(hWnd);
      */
      break;

   case WM_DESTROY:
      if (ghDC)
         ReleaseDC(hWnd, ghDC);

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

   auto windowWidth = 1920 / 2;
   auto windowHeight = (854 / 2) + (480 / 2);

   /* Create the frame */
   ghWnd = CreateWindow(szAppName,
      szAppTitle,
      WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      windowWidth,
      windowHeight,
      NULL,
      NULL,
      hInstance,
      NULL);

   /* show and update main window */
   ShowWindow(ghWnd, 1);

   UpdateWindow(ghWnd);

   ghDC = GetDC(ghWnd);
   if (!bSetupPixelFormat(ghDC))
      PostQuitMessage(0);

   for (auto i = 0; i < 3; ++i) {
      ghRC[i] = (HGLRC)INVALID_HANDLE_VALUE;
   }
}

void initialiseCore(int coreId)
{
   auto baseRC = (HGLRC)INVALID_HANDLE_VALUE;
   bool madeCurrent = false;

   for (auto i = 0; i < 3; ++i) {
      if (ghRC[i] != INVALID_HANDLE_VALUE) {
         baseRC = ghRC[i];
         break;
      }
   }

   if (ghRC[coreId] != INVALID_HANDLE_VALUE) {
      assert(0);
   }

   ghRC[coreId] = wglCreateContext(ghDC);

   if (baseRC != INVALID_HANDLE_VALUE) {
      if (wglShareLists(baseRC, ghRC[coreId]) == FALSE) {
         assert(0);
      }
   }

   for (auto i = 0u; i < 10; ++i) {
      if (wglMakeCurrent(ghDC, ghRC[coreId]) == TRUE) {
         madeCurrent = true;
         break;
      }

      Sleep(10);
   }

   assert(madeCurrent);
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

int drcWidth() { return 854; }
int drcHeight() { return 480; }
int tvWidth() { return 1920;  }
int tvHeight() { return 1080; }

}

}

#endif
