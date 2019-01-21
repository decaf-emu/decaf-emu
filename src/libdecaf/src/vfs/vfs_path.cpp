#include "vfs_path.h"
#include "vfs_pathiterator.h"

#include <algorithm>
#include <list>
#include <string_view>

namespace vfs
{

static std::string normalisePath(std::string_view path);

Path::Path(const std::string &path) :
   mPath(normalisePath(std::string_view { path }))
{
}

Path::Path(const char *path) :
   mPath(normalisePath(std::string_view { path }))
{
}

Path::Path(std::string_view path) :
   mPath(normalisePath(path))
{
}

Path::Path(PathIterator first, PathIterator last) :
   mPath()
{
   auto size = static_cast<size_t>(std::distance(first.mPosition, last.mPosition));
   if (size) {
      mPath = normalisePath(std::string_view { &*first.mPosition, size });
   }
}

void
Path::clear()
{
   mPath.clear();
}

size_t
Path::depth() const
{
   if (mPath.empty()) {
      return 0;
   }

   return 1 + std::count(mPath.begin(), mPath.end(), '/');
}

bool
Path::empty() const
{
   return mPath.empty();
}

const std::string &
Path::path() const
{
   return mPath;
}

PathIterator
Path::begin() const
{
   if (!mPath.empty()) {
      auto firstBegin = mPath.begin();
      auto firstEnd = std::find(mPath.begin(), mPath.end(), '/');

      if (firstBegin == firstEnd) {
         return PathIterator { this, mPath.begin(),
                               std::string_view { &*firstBegin, 1 } };
      } else {
         auto size = static_cast<size_t>(firstEnd - firstBegin);
         return PathIterator { this, mPath.begin(),
                               std::string_view { &*firstBegin, size } };
      }
   }

   return PathIterator { this, mPath.begin() };
}

PathIterator
Path::end() const
{
   return PathIterator { this, mPath.end() };
}

Path
Path::operator /(const Path &path) const
{
   return Path(mPath + "/" + path.mPath);
}

std::string
normalisePath(std::string_view path)
{
   // 1) If the path is empty, stop
   if (path.empty()) {
      return {};
   }

   // 2) Replace each directory-separator (which may consist of multiple
   //    slashes) with a single path::preferred_separator.
   auto expanded = std::list<std::string_view> { };
   for (auto itr = path.begin(); itr != path.end(); ) {
      if (*itr == '/') {
         if (expanded.empty() || !expanded.back().empty()) {
            expanded.emplace_back();
         }

         ++itr;
      } else {
         auto end = std::find(itr + 1, path.end(), '/');
         expanded.emplace_back(&*itr, static_cast<size_t>(end - itr));
         itr = end;
      }
   }

   // 3) Replace each slash character in the root-name with
   //    path::preferred_separator.
   // Nothing to do for us.

   // 4) Remove each dot and any immediately following directory-separator.
   for (auto itr = expanded.begin(); itr != expanded.end(); ) {
      if (*itr == ".") {
         // Erase dot filename.
         itr = expanded.erase(itr);

         if (itr != expanded.end()) {
            // Erase immediately following directory-separator.
            itr = expanded.erase(itr);
         }
      } else {
         ++itr;
      }
   }

   // 5) Remove each non-dot-dot filename immediately followed by a
   //    directory-separator and a dot-dot, along with any immediately
   //    following directory-separator.
   for (auto itr = expanded.begin(); itr != expanded.end(); ) {
      auto start = itr;
      ++itr;

      if (*start == ".." &&
            start != expanded.begin() &&
            --start != expanded.begin() &&
            *--start != "..") {
         if (itr != expanded.end()) {
            // dot-dot filename has following directory-separator
            ++itr;
         }

         expanded.erase(start, itr);
      }
   }

   // 6) If there is root-directory, remove all dot-dots and any
   //    directory-separators immediately following them.
   if (!expanded.empty() && expanded.front().empty()) {
      for (auto itr = expanded.begin(); itr != expanded.end(); ) {
         if (*itr == "..") {
            // Erase dot-dot filename.
            itr = expanded.erase(itr);

            if (itr != expanded.end()) {
               // Erase immediately following directory-separator.
               itr = expanded.erase(itr);
            }
         } else {
            break;
         }
      }
   }

   // 7) If the last filename is dot-dot, remove any trailing directory-separator.
   if (expanded.size() >= 2 &&
         expanded.back().empty() &&
         *prev(expanded.end(), 2) == "..") {
      expanded.pop_back();
   }

   // Remove trailing separator
   while (expanded.size() > 1 && expanded.back().empty()) {
      expanded.pop_back();
   }

   // Flatten into normalised path
   std::string normalised;

   for (auto itr = expanded.begin(); itr != expanded.end(); ++itr) {
      if (itr->empty()) {
         normalised.push_back('/');
      } else {
         normalised.append(*itr);
      }
   }

   // 8) If the path is empty, add a dot (normal form of ./ is .)
   if (normalised.empty()) {
      normalised = ".";
   }

   return std::move(normalised);
}

} // namespace vfs
