#pragma once
#include "ios_fs_enum.h"
#include "ios_fs_fsa_types.h"

#include <cstdint>
#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::fs
{

/**
 * \ingroup ios_fs
 * @{
 */

#pragma pack(push, 1)

struct FSAResponseGetCwd
{
   be2_array<char, FSAPathLength> path;
};
CHECK_OFFSET(FSAResponseGetCwd, 0x0, path);
CHECK_SIZE(FSAResponseGetCwd, 0x280);

struct FSAResponseGetFileBlockAddress
{
   be2_val<uint32_t> address;
};
CHECK_OFFSET(FSAResponseGetFileBlockAddress, 0x0, address);
CHECK_SIZE(FSAResponseGetFileBlockAddress, 0x4);

struct FSAResponseGetPosFile
{
   be2_val<FSAFilePosition> pos;
};
CHECK_OFFSET(FSAResponseGetPosFile, 0x0, pos);
CHECK_SIZE(FSAResponseGetPosFile, 0x4);

struct FSAResponseGetVolumeInfo
{
   be2_struct<FSAVolumeInfo> volumeInfo;
};
CHECK_OFFSET(FSAResponseGetVolumeInfo, 0x0, volumeInfo);
CHECK_SIZE(FSAResponseGetVolumeInfo, 0x1BC);

union FSAResponseGetInfoByQuery
{
   be2_val<uint64_t> dirSize;
   be2_val<FSAEntryNum> entryNum;
   be2_val<uint64_t> freeSpaceSize;
   be2_struct<FSAFileSystemInfo> fileSystemInfo;
   be2_struct<FSAStat> stat;
};
CHECK_OFFSET(FSAResponseGetInfoByQuery, 0x0, dirSize);
CHECK_OFFSET(FSAResponseGetInfoByQuery, 0x0, entryNum);
CHECK_OFFSET(FSAResponseGetInfoByQuery, 0x0, freeSpaceSize);
CHECK_OFFSET(FSAResponseGetInfoByQuery, 0x0, stat);
CHECK_SIZE(FSAResponseGetInfoByQuery, 0x64);

struct FSAResponseOpenFile
{
   be2_val<FSAFileHandle> handle;
};
CHECK_OFFSET(FSAResponseOpenFile, 0x0, handle);
CHECK_SIZE(FSAResponseOpenFile, 0x4);

struct FSAResponseOpenDir
{
   be2_val<FSADirHandle> handle;
};
CHECK_OFFSET(FSAResponseOpenDir, 0x0, handle);
CHECK_SIZE(FSAResponseOpenDir, 0x4);

struct FSAResponseReadDir
{
   be2_struct<FSADirEntry> entry;
};
CHECK_OFFSET(FSAResponseReadDir, 0x0, entry);
CHECK_SIZE(FSAResponseReadDir, 0x164);

struct FSAResponseStatFile
{
   be2_struct<FSAStat> stat;
};
CHECK_OFFSET(FSAResponseStatFile, 0x0, stat);
CHECK_SIZE(FSAResponseStatFile, 0x64);

struct FSAResponse
{
   be2_val<uint32_t> word0;

   union
   {
      be2_struct<FSAResponseGetCwd> getCwd;
      be2_struct<FSAResponseGetFileBlockAddress> getFileBlockAddress;
      be2_struct<FSAResponseGetPosFile> getPosFile;
      be2_struct<FSAResponseGetVolumeInfo> getVolumeInfo;
      be2_struct<FSAResponseGetInfoByQuery> getInfoByQuery;
      be2_struct<FSAResponseOpenDir> openDir;
      be2_struct<FSAResponseOpenFile> openFile;
      be2_struct<FSAResponseReadDir> readDir;
      be2_struct<FSAResponseStatFile> statFile;
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

} // namespace ios::fs
