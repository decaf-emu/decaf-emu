#pragma once
#include "platform.h"

#ifdef PLATFORM_WINDOWS
#include <string>
#include <Windows.h>

namespace platform
{

/**
 * Convert from UTF-8 string to UTF-16 string expected by Windows API.
 */
static inline std::wstring
toWinApiString(const std::string &utf8)
{
   auto result = std::wstring { };
   auto size = MultiByteToWideChar(CP_UTF8, 0,
                                   utf8.data(), static_cast<int>(utf8.size()),
                                   NULL, 0);
   result.resize(size);
   MultiByteToWideChar(CP_UTF8, 0,
                       utf8.data(), static_cast<int>(utf8.size()),
                       result.data(), static_cast<int>(result.size()));
   return result;
}

/**
 * Convert to UTF-8 string from UTF-16 string returned by Windows API.
 */
static inline std::string
fromWinApiString(const std::wstring &utf16)
{
   auto result = std::string { };
   auto size = WideCharToMultiByte(CP_UTF8, 0,
                                   utf16.data(), static_cast<int>(utf16.size()),
                                   NULL, 0,
                                   NULL, NULL);
   result.resize(size);
   WideCharToMultiByte(CP_UTF8, 0,
                       utf16.data(), static_cast<int>(utf16.size()),
                       result.data(), static_cast<int>(result.size()),
                       NULL, NULL);
   return result;
}

} // namespace platform

#endif // ifdef PLATFORM_WINDOWS
