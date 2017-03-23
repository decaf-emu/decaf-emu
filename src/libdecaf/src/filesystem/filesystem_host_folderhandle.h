#pragma once
#include "filesystem_folderhandle.h"
#include "filesystem_host_path.h"

namespace fs
{

class HostFolderHandle : public IFolderHandle
{
public:
   HostFolderHandle(const HostPath &path) :
      mPath(path),
      mFindData(nullptr)
   {
   }

   virtual ~HostFolderHandle() override
   {
      close();
   }

   virtual bool
   open() override;

   virtual void
   close() override;

   virtual bool
   read(FolderEntry &entry) override;

   virtual bool
   rewind() override;

private:
   HostPath mPath;
   void *mFindData;
};

} // namespace fs
