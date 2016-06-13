#pragma once
#include "filesystem_file.h"
#include "filesystem_virtual_filedata.h"
#include "filesystem_virtual_filehandle.h"

namespace fs
{

class VirtualFile : public File
{
public:
   VirtualFile(const std::string &name) :
      File(name)
   {
   }

   virtual ~VirtualFile() override = default;

   FileHandle *open(OpenMode mode) override
   {
      auto handle = new VirtualFileHandle(mData, mode);

      if (!handle->open()) {
         delete handle;
         return nullptr;
      }

      return handle;
   }

private:
   VirtualFileData mData;
};

} // namespace fs
