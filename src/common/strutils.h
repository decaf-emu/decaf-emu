#pragma once
#include "platform.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#ifdef PLATFORM_WINDOWS
   #define snprintf sprintf_s
   #define vsnprintf vsprintf_s
   #define strdup _strdup
#endif

// Replace all occurences of a character in a string
inline void
replace_all(std::string &source, char find, char replace)
{
   std::string::size_type offset = 0;

   while ((offset = source.find(find, offset)) != std::string::npos) {
      source[offset] = replace;
      ++offset;
   }
}

// Split a string by a character
inline void
split_string(const std::string &source, char delimiter, std::vector<std::string> &result)
{
   std::string::size_type offset = 0, last = 0;

   if (source.at(0) == delimiter) {
      offset = last = 1;
   }

   while ((offset = source.find(delimiter, offset)) != std::string::npos) {
      if (offset - last > 0) {
         result.push_back(source.substr(last, offset - last));
      }

      last = ++offset;
   }

   result.push_back(source.substr(last, offset - last));
}

// Joins a vector of strings into one string using a delimeter
template<typename IteratorType>
inline void
join_string(IteratorType begin, IteratorType end, char delim, std::string &out)
{
   bool first = true;

   for (auto itr = begin; itr != end; ++itr) {
      if (first) {
         out += *itr;
         first = false;
      } else {
         out += delim + *itr;
      }
   }
}

// Returns true if source begins with prefix
inline bool
begins_with(const std::string &source, const std::string &prefix)
{
   if (prefix.size() > source.size()) {
      return false;
   } else {
      return std::equal(prefix.begin(), prefix.end(), source.begin());
   }
}

// Returns true if source ends with suffix
inline bool
ends_with(const std::string &source, const std::string &suffix)
{
   if (suffix.size() > source.size()) {
      return false;
   } else {
      return std::equal(suffix.rbegin(), suffix.rend(), source.rbegin());
   }
}

// Creates a string according to a format string and varargs
static inline std::string
format_string(const char* fmt, ...)
{
   va_list args, args2;
   std::string ret;

   va_start(args, fmt);
   va_copy(args2, args);

   // Calculate the size for our char buffer
#ifdef PLATFORM_WINDOWS
   int formatted_len = _vscprintf(fmt, args2);
#else
   int formatted_len = vsnprintf(nullptr, 0, fmt, args2);
#endif
   va_end(args2);

   if (formatted_len > 0) {
      // Reserve space for the trailing null as well (C++11 doesn't require
      // that implementations include a trailing null element)
      ret.resize(formatted_len + 1);

      // C++11 guarantees that strings are contiguous in memory
#ifdef PLATFORM_WINDOWS
      vsprintf_s(&ret[0], formatted_len + 1, fmt, args);
#else
      vsnprintf(&ret[0], formatted_len + 1, fmt, args);
#endif
   }
   va_end(args);

   ret.resize(formatted_len);
   return ret;
}
