#pragma once
#include "filesystem_file.h"

namespace fs
{

class VirtualFile : public File
{
public:
   virtual ~VirtualFile()
   {
   }

   FileHandle *open(OpenMode mode) override
   {
      // TODO: Support virtual files.
      return nullptr;
   }
};

} // namespace fs
