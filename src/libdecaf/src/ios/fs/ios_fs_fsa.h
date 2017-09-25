#pragma once
#include "ios_fs_enum.h"
#include "ios_fs_fsa_request.h"
#include "ios_fs_fsa_response.h"
#include "ios_fs_fsa_types.h"
#include "ios/ios_device.h"
#include "kernel/kernel_filesystem.h"

#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::fs
{

/**
 * \ingroup ios_fs
 * @{
 */

class FSADevice
{
   using FileSystem = ::fs::FileSystem;
   using FileHandle = ::fs::FileHandle;
   using FolderHandle = ::fs::FolderHandle;
   using Path = ::fs::Path;

   struct Handle
   {
      enum Type
      {
         Unused,
         File,
         Folder
      };

      Type type;
      ::fs::FileHandle file;
      ::fs::FolderHandle folder;
   };

public:
   FSAStatus getCwd(FSAResponseGetCwd &response);

private:
   FileSystem *mFS = nullptr;
   Path mWorkingPath = "/";
   std::vector<Handle> mHandles;
};

/** @} */

} // namespace ios::fs
