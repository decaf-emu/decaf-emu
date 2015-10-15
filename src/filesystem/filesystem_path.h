#pragma once
#include "strutils.h"

namespace fs
{

struct FilePath
{
   FilePath(const char *path) :
      FilePath(std::string { path })
   {
   }

   FilePath(std::string pathStr) :
      path(pathStr)
   {
      if (path.at(0) == '/') {
         exploded.push_back("/");
      }

      split_string(path, '/', exploded);
   }

   const std::string &name() const
   {
      return exploded.back();
   }

   std::string path;
   std::vector<std::string> exploded;
};

using FilePathIterator = std::vector<std::string>::const_iterator;

} // namespace fs
