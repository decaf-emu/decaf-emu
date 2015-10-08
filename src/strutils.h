#pragma once
#include <algorithm>
#include <string>
#include <vector>

// Replace all occurences of a character in a string
static inline void
replace_all(std::string &source, char find, char replace)
{
   std::string::size_type offset = 0;

   while ((offset = source.find(find, offset)) != std::string::npos) {
      source[offset] = replace;
      ++offset;
   }
}

// Split a string by a character
static inline void
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

// Returns true if source ends with suffix
static inline bool
ends_with(const std::string &source, const std::string &suffix)
{
   if (suffix.size() > source.size()) {
      return false;
   } else {
      return std::equal(suffix.rbegin(), suffix.rend(), source.rbegin());
   }
}
