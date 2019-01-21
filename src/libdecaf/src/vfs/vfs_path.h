#pragma once
#include <string>

namespace vfs
{

class PathIterator;

class Path
{
   friend class PathIterator;

public:
   Path() = default;
   Path(const std::string &path);
   Path(const char *path);
   Path(std::string_view path);
   Path(PathIterator first, PathIterator last);

   void clear();
   size_t depth() const;
   bool empty() const;
   const std::string &path() const;

   PathIterator begin() const;
   PathIterator end() const;

   Path operator /(const Path &path) const;

private:
   std::string mPath;
};

} // namespace vfs
