#pragma once
#include <string>

namespace fs
{

struct FolderEntry
{
   enum Type
   {
      Unknown,
      File,
      Folder,
   };

   std::string name;
   size_t size;
   Type type = Unknown;
};

class FolderHandle
{
public:
   virtual ~FolderHandle() = default;

   virtual bool open() = 0;
   virtual void close() = 0;

   virtual bool read(FolderEntry &entry) = 0;
   virtual bool rewind() = 0;
};

} // namespace fs
