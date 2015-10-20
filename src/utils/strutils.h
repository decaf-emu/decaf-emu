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

// Joins a vector of strings into one string using a delimeter
template<typename IteratorType>
static inline void
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
static inline bool
begins_with(const std::string &source, const std::string &prefix)
{
   if (prefix.size() > source.size()) {
      return false;
   } else {
      return std::equal(prefix.begin(), prefix.end(), source.begin());
   }
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
