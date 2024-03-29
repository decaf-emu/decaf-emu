#pragma once
#include "platform.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#ifdef PLATFORM_WINDOWS
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
split_string(const std::string_view &source,
             char delimiter,
             std::vector<std::string> &result)
{
   std::string_view::size_type offset = 0, last = 0;

   if (source.at(0) == delimiter) {
      offset = last = 1;
   }

   while ((offset = source.find(delimiter, offset)) != std::string::npos) {
      if (offset - last > 0) {
         result.push_back(std::string { source.substr(last, offset - last) });
      }

      last = ++offset;
   }

   result.push_back(std::string { source.substr(last, offset - last) });
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
begins_with(const std::string_view &source, const std::string_view &prefix)
{
   if (prefix.size() > source.size()) {
      return false;
   } else {
      return std::equal(prefix.begin(), prefix.end(), source.begin());
   }
}

// Returns true if source ends with suffix
inline bool
ends_with(const std::string_view &source, const std::string_view &suffix)
{
   if (suffix.size() > source.size()) {
      return false;
   } else {
      return std::equal(suffix.rbegin(), suffix.rend(), source.rbegin());
   }
}

// Case insensitive string compare
inline bool
iequals(const std::string_view &a, const std::string_view &b)
{
   return std::equal(a.begin(), a.end(),
                     b.begin(), b.end(),
                     [](char a, char b) {
                        return tolower(a) == tolower(b);
                     });
}

// A strncpy which does not warn on Windows
inline void
string_copy(char *dst,
            size_t dstSize,
            const char *src,
            size_t maxCount)
{
#ifdef PLATFORM_WINDOWS
   strncpy_s(dst, dstSize, src, maxCount);
#else
   std::strncpy(dst, src, dstSize);
#endif
}

inline void
string_copy(char *dst,
            const char *src,
            size_t maxCount)
{
   string_copy(dst, maxCount, src, maxCount);
}

template<typename CharT, typename DstCharT, typename SrcCharT>
inline void
string_copy(DstCharT *dst,
            size_t dstSize,
            const SrcCharT *src,
            size_t maxCount)
{
   if (dstSize <= maxCount) {
      maxCount = dstSize - 1;
   }

   auto i = size_t { 0 };
   while (src[i] && i < maxCount) {
      dst[i] = static_cast<DstCharT>(src[i]);
      i++;
   }

   dst[i] = static_cast<CharT>('\0');
}

inline std::string_view
trim_view(std::string_view str)
{
   while (!str.empty() && std::isspace(str[0])) {
      str.remove_prefix(1);
   }

   while (!str.empty() && std::isspace(str.back())) {
      str.remove_suffix(1);
   }

   return str;
}

inline std::string
trim(std::string_view str)
{
   while (!str.empty() && std::isspace(str[0])) {
      str.remove_prefix(1);
   }

   while (!str.empty() && std::isspace(str.back())) {
      str.remove_suffix(1);
   }

   return std::string { str };
}
