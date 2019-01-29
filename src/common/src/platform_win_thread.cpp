#include "platform.h"

#ifdef PLATFORM_WINDOWS
#include "platform_thread.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)

typedef struct tagTHREADNAME_INFO
{
   DWORD dwType;     // Must be 0x1000.
   LPCSTR szName;    // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;

#pragma pack(pop)

namespace platform
{

void
setThreadName(std::thread *thread,
              const std::string &threadName)
{
   DWORD dwThreadID = ::GetThreadId(static_cast<HANDLE>(thread->native_handle()));

   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = threadName.c_str();
   info.dwThreadID = dwThreadID;
   info.dwFlags = 0;

   __try {
      RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
   } __except (EXCEPTION_EXECUTE_HANDLER)
   {
   }
}

void
exitThread(int result)
{
   ExitThread(result);
}

} // namespace platform

#endif
