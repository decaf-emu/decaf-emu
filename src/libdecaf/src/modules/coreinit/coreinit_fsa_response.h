#pragma once
#include "coreinit_enum.h"
#include "coreinit_fs.h"
#include "coreinit_fsa.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

namespace coreinit
{

/**
 * \ingroup coreinit_fsa
 * @{
 */

#pragma pack(push, 1)

struct FSAResponseGetCwd
{
   char path[FSMaxPathLength + 1];
};
CHECK_OFFSET(FSAResponseGetCwd, 0x0, path);
CHECK_SIZE(FSAResponseGetCwd, 0x280);

struct FSAResponseGetFileBlockAddress
{
   be_val<uint32_t> address;
};
CHECK_OFFSET(FSAResponseGetFileBlockAddress, 0x0, address);
CHECK_SIZE(FSAResponseGetFileBlockAddress, 0x4);

struct FSAResponseGetPosFile
{
   be_val<FSFilePosition> pos;
};
CHECK_OFFSET(FSAResponseGetPosFile, 0x0, pos);
CHECK_SIZE(FSAResponseGetPosFile, 0x4);

struct FSAResponseGetVolumeInfo
{
   FSAVolumeInfo volumeInfo;
};
CHECK_OFFSET(FSAResponseGetVolumeInfo, 0x0, volumeInfo);
CHECK_SIZE(FSAResponseGetVolumeInfo, 0x1BC);

union FSAResponseGetInfoByQuery
{
   be_val<uint64_t> dirSize;
   be_val<FSEntryNum> entryNum;
   be_val<uint64_t> freeSpaceSize;
   FSFileSystemInfo fileSystemInfo;
   FSStat stat;
};
CHECK_OFFSET(FSAResponseGetInfoByQuery, 0x0, dirSize);
CHECK_OFFSET(FSAResponseGetInfoByQuery, 0x0, entryNum);
CHECK_OFFSET(FSAResponseGetInfoByQuery, 0x0, freeSpaceSize);
CHECK_OFFSET(FSAResponseGetInfoByQuery, 0x0, stat);
CHECK_SIZE(FSAResponseGetInfoByQuery, 0x64);

struct FSAResponseOpenFile
{
   be_val<FSFileHandle> handle;
};
CHECK_OFFSET(FSAResponseOpenFile, 0x0, handle);
CHECK_SIZE(FSAResponseOpenFile, 0x4);

struct FSAResponseOpenDir
{
   be_val<FSDirHandle> handle;
};
CHECK_OFFSET(FSAResponseOpenDir, 0x0, handle);
CHECK_SIZE(FSAResponseOpenDir, 0x4);

struct FSAResponseReadDir
{
   FSDirEntry entry;
};
CHECK_OFFSET(FSAResponseReadDir, 0x0, entry);
CHECK_SIZE(FSAResponseReadDir, 0x164);

struct FSAResponseStatFile
{
   FSStat stat;
};
CHECK_OFFSET(FSAResponseStatFile, 0x0, stat);
CHECK_SIZE(FSAResponseStatFile, 0x64);

struct FSAResponse
{
   be_val<uint32_t> word0;

   union
   {
      FSAResponseGetCwd getCwd;
      FSAResponseGetFileBlockAddress getFileBlockAddress;
      FSAResponseGetPosFile getPosFile;
      FSAResponseGetVolumeInfo getVolumeInfo;
      FSAResponseGetInfoByQuery getInfoByQuery;
      FSAResponseOpenDir openDir;
      FSAResponseOpenFile openFile;
      FSAResponseReadDir readDir;
      FSAResponseStatFile statFile;
      UNKNOWN(0x28F);
   };
};
CHECK_OFFSET(FSAResponse, 0x0, word0);
CHECK_OFFSET(FSAResponse, 0x4, getFileBlockAddress);
CHECK_OFFSET(FSAResponse, 0x4, getPosFile);
CHECK_OFFSET(FSAResponse, 0x4, getVolumeInfo);
CHECK_OFFSET(FSAResponse, 0x4, openDir);
CHECK_OFFSET(FSAResponse, 0x4, openFile);
CHECK_OFFSET(FSAResponse, 0x4, readDir);
CHECK_OFFSET(FSAResponse, 0x4, statFile);
CHECK_SIZE(FSAResponse, 0x293);

#pragma pack(pop)

/** @} */

} // namespace coreinit
