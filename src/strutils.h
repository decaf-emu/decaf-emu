#pragma once
#include <string>
#include <vector>

static inline void
replaceAll(std::string &source, char find, char replace)
{
   std::string::size_type offset = 0;

   while ((offset = source.find(find, offset)) != std::string::npos) {
      source[offset] = replace;
      ++offset;
   }
}

static inline void
splitString(const std::string &source, char delimiter, std::vector<std::string> &result)
{
   std::string::size_type offset = 0, last = 0;

   if (source.at(0) == delimiter) {
      offset = last = 1;
   }

   while ((offset = source.find(delimiter, offset)) != std::string::npos) {
      result.push_back(source.substr(last, offset - last));
      last = ++offset;
   }

   result.push_back(source.substr(last, offset - last));
}
