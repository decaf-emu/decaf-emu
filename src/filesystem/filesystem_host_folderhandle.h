#pragma once
#include "filesystem_folderhandle.h"
#include "filesystem_host_path.h"

namespace fs
{

class HostFolderHandle : public FolderHandle
{
public:
   HostFolderHandle(const HostPath &path, FolderHandle *virtualHandle) :
      mPath(path),
      mVirtualHandle(virtualHandle),
      mFindData(nullptr)
   {
   }

   virtual ~HostFolderHandle()
   {
      close();
      delete mVirtualHandle;
   }

   virtual bool open() override;
   virtual void close() override;

   virtual bool read(FolderEntry &entry) override;
   virtual bool rewind() override;

private:
   HostPath mPath;
   FolderHandle *mVirtualHandle;
   void *mFindData;
};

} // namespace fs
