#pragma once
#include "strutils.h"

namespace fs
{

class HostPath
{
public:
   HostPath()
   {
   }

   HostPath(std::string pathStr) :
      mPath(pathStr)
   {
      replace_all(mPath, '/', '\\');
      split_string(mPath, '\\', mExploded);
   }

   HostPath join(const std::string &rhs) const
   {
      HostPath result;
      result.mPath = mPath + '\\' + rhs;
      result.mExploded = mExploded;
      result.mExploded.push_back(rhs);
      return result;
   }

   const std::string &path() const
   {
      return mPath;
   }

   const std::string &name() const
   {
      return mExploded.back();
   }

   operator const char*() const
   {
      return mPath.c_str();
   }

   operator const std::string&() const
   {
      return mPath;
   }

private:
   std::string mPath;
   std::vector<std::string> mExploded;
};

} // namespace fs
