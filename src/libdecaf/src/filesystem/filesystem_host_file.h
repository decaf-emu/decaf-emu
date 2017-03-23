#pragma once
#include "filesystem_file.h"
#include "filesystem_host_filehandle.h"
#include "filesystem_host_path.h"

#include <string>
#include <memory>

namespace fs
{

class HostFile : public File
{
public:
   HostFile(const HostPath &path,
            const std::string &name,
            Permissions permissions) :
      File(DeviceType::HostDevice, permissions, name),
      mPath(path)
   {

   }

   virtual ~HostFile() override = default;

   virtual FileHandle
   open(OpenMode mode) override
   {
      if (!checkOpenPermissions(mode)) {
         return nullptr;
      }

      auto handle = new HostFileHandle { mPath.path(), mode };

      if (!handle->open()) {
         delete handle;
         return nullptr;
      }

      return FileHandle { handle };
   }

private:
   HostPath mPath;
};

} // namespace fs
