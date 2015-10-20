#pragma once
#include <string_view.h>
#include "strutils.h"

namespace fs
{

// TODO: Change exploded to use gsl::string_view
struct FilePath
{
   FilePath(const char *path) :
      FilePath(std::string { path })
   {
   }

   FilePath(const std::string &src)
   {
      setPath(src);
   }

   void setPath(const std::string &src)
   {
      // Parse path into exploded
      if (src.at(0) == '/') {
         exploded.push_back("/");
      }

      split_string(src, '/', exploded);

      // Process any relative directories by erasing parent
      for (auto itr = exploded.begin(); itr != exploded.end(); ++itr) {
         if (itr->compare("..") == 0 && itr != exploded.begin()) {
            itr = exploded.erase(itr);
            itr = exploded.erase(itr - 1);
         }
      }

      // Recombine exploded to create path string
      if (!exploded.empty() && exploded[0].compare("/") == 0) {
         path = '/';
         join_string(exploded.begin() + 1, exploded.end(), '/', path);
      } else {
         join_string(exploded.begin(), exploded.end(), '/', path);
      }
   }

   const FilePath &operator= (const std::string &src)
   {
      setPath(src);
      return *this;
   }

   const FilePath &operator+= (const FilePath &rhs)
   {
      setPath(path + '/' + rhs.path);
      return *this;
   }

   FilePath operator+ (const FilePath &rhs) const
   {
      return FilePath { path + '/' + rhs.path };
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
