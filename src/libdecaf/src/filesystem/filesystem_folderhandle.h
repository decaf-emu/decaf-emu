#pragma once
#include <string>
#include <memory>

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
   size_t size = 0;
   Type type = Unknown;
};

class IFolderHandle
{
public:
   virtual ~IFolderHandle() = default;

   virtual bool
   open() = 0;

   virtual void
   close() = 0;

   virtual bool
   read(FolderEntry &entry) = 0;

   virtual bool
   rewind() = 0;
};

using FolderHandle = std::shared_ptr<IFolderHandle>;

} // namespace fs
