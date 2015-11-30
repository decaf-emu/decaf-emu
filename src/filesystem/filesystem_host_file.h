#pragma once
#include <string>
#include "filesystem_file.h"
#include "filesystem_host_filehandle.h"
#include "filesystem_host_path.h"

namespace fs
{

class HostFile : public File
{
public:
   HostFile(const HostPath &path, const std::string &name) :
      File(name),
      mPath(path)
   {

   }

   virtual ~HostFile()
   {
   }

   virtual class FileHandle *open(OpenMode mode)
   {
      auto handle = new HostFileHandle(mPath.path(), mode);

      if (!handle->open()) {
         delete handle;
         return nullptr;
      }

      return handle;
   }

private:
   HostPath mPath;
};

} // namespace fs
