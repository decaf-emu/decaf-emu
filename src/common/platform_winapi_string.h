#pragma once
#include <codecvt>
#include <locale>
#include <string>

namespace platform
{

/**
 * Convert from UTF-8 string to UTF-16 string expected by Windows API.
 */
static inline std::wstring
toWinApiString(const std::string &utf8)
{
   std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
   return converter.from_bytes(utf8);
}

/**
 * Convert to UTF-8 string from UTF-16 string returned by Windows API.
 */
static inline std::string
fromWinApiString(const std::wstring &utf16)
{
   std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
   return converter.to_bytes(utf16);
}

} // namespace platform
