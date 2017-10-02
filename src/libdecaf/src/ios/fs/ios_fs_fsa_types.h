#pragma once
#include "ios_fs_enum.h"

#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::fs
{

/**
 * \ingroup ios_fs
 * @{
 */

#pragma pack(push, 1)


constexpr auto FSAPathLength = 640u;
constexpr auto FSAModeLength = 16u;
constexpr auto FSAFileNameLength = 256u;
constexpr auto FSAVolumeLabelLength = 128u;
constexpr auto FSAVolumeIdLength = 128u;
constexpr auto FSADevicePathLength = 16u;
constexpr auto FSAMountPathLength = 128u;


using FSADeviceHandle = int32_t;
using FSADirHandle = int32_t;
using FSAEntryNum = int32_t;
using FSAFileHandle = int32_t;
using FSAFilePosition = uint32_t;


/**
 * Block information.
 */
struct FSABlockInfo
{
   UNKNOWN(0x14);
};
CHECK_SIZE(FSABlockInfo, 0x14);


/**
 * Device information.
 */
struct FSADeviceInfo
{
   UNKNOWN(0x28);
};
CHECK_SIZE(FSADeviceInfo, 0x28);


/**
 * File System information.
 */
struct FSAFileSystemInfo
{
   UNKNOWN(0x1E);
};
CHECK_SIZE(FSAFileSystemInfo, 0x1E);


/**
 * Information about a file or directory.
 */
struct FSAStat
{
   be2_val<FSAStatFlags> flags;
   be2_val<uint32_t> permission;
   be2_val<uint32_t> owner;
   be2_val<uint32_t> group;
   be2_val<uint32_t> size;
   UNKNOWN(0xC);
   be2_val<uint32_t> entryId;
   be2_val<int64_t> created;
   be2_val<int64_t> modified;
   UNKNOWN(0x30);
};
CHECK_OFFSET(FSAStat, 0x00, flags);
CHECK_OFFSET(FSAStat, 0x04, permission);
CHECK_OFFSET(FSAStat, 0x08, owner);
CHECK_OFFSET(FSAStat, 0x0C, group);
CHECK_OFFSET(FSAStat, 0x10, size);
CHECK_OFFSET(FSAStat, 0x20, entryId);
CHECK_OFFSET(FSAStat, 0x24, created);
CHECK_OFFSET(FSAStat, 0x2C, modified);
CHECK_SIZE(FSAStat, 0x64);


/**
 * Information about an item in a directory.
 */
struct FSADirEntry
{
   //! File stat.
   be2_struct<FSAStat> stat;

   //! File name.
   be2_array<char, FSAFileNameLength> name;
};
CHECK_OFFSET(FSADirEntry, 0x00, stat);
CHECK_OFFSET(FSADirEntry, 0x64, name);
CHECK_SIZE(FSADirEntry, 0x164);


/**
 * Volume information.
 */
struct FSAVolumeInfo
{
   be2_val<uint32_t> flags;
   be2_val<FSAMediaState> mediaState;
   UNKNOWN(0x4);
   be2_val<uint32_t> unk0x0C;
   be2_val<uint32_t> unk0x10;
   be2_val<int32_t> unk0x14;
   be2_val<int32_t> unk0x18;
   UNKNOWN(0x10);
   be2_array<char, FSAVolumeLabelLength> volumeLabel;
   be2_array<char, FSAVolumeIdLength> volumeId;
   be2_array<char, FSADevicePathLength> devicePath;
   be2_array<char, FSAMountPathLength> mountPath;
};
CHECK_OFFSET(FSAVolumeInfo, 0x00, flags);
CHECK_OFFSET(FSAVolumeInfo, 0x04, mediaState);
CHECK_OFFSET(FSAVolumeInfo, 0x0C, unk0x0C);
CHECK_OFFSET(FSAVolumeInfo, 0x10, unk0x10);
CHECK_OFFSET(FSAVolumeInfo, 0x14, unk0x14);
CHECK_OFFSET(FSAVolumeInfo, 0x18, unk0x18);
CHECK_OFFSET(FSAVolumeInfo, 0x2C, volumeLabel);
CHECK_OFFSET(FSAVolumeInfo, 0xAC, volumeId);
CHECK_OFFSET(FSAVolumeInfo, 0x12C, devicePath);
CHECK_OFFSET(FSAVolumeInfo, 0x13C, mountPath);
CHECK_SIZE(FSAVolumeInfo, 0x1BC);

#pragma pack(pop)

/** @} */

} // namespace ios::fs
