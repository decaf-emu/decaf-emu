#pragma once
#include "fsa_enum.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/structsize.h>

namespace kernel
{

namespace ios
{

namespace fsa
{

/**
* \ingroup kernel_ios_fsa
* @{
*/

#pragma pack(push, 1)

static constexpr auto FSAMaxPathLength = 0x27F;

using FSADirHandle = uint32_t;
using FSAEntryNum = uint32_t;
using FSAFileHandle = uint32_t;
using FSAFilePosition = uint32_t;


/**
 * File System information.
 */
struct FSAFileSystemInfo
{
   UNKNOWN(0x1C);
};
CHECK_SIZE(FSAFileSystemInfo, 0x1C);


/**
 * Information about a file or directory.
 */
struct FSAStat
{
   be_val<FSAStatFlags> flags;
   be_val<uint32_t> permission;
   be_val<uint32_t> owner;
   be_val<uint32_t> group;
   be_val<uint32_t> size;
   UNKNOWN(0xC);
   be_val<uint32_t> entryId;
   be_val<int64_t> created;
   be_val<int64_t> modified;
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
   FSAStat stat;

   //! File name.
   char name[256];
};
CHECK_OFFSET(FSADirEntry, 0x00, stat);
CHECK_OFFSET(FSADirEntry, 0x64, name);
CHECK_SIZE(FSADirEntry, 0x164);


/**
 * Volume information.
 */
struct FSAVolumeInfo
{
   be_val<uint32_t> flags;
   be_val<FSAMediaState> mediaState;
   UNKNOWN(0x4);
   be_val<uint32_t> unk0x0C;
   be_val<uint32_t> unk0x10;
   be_val<int32_t> unk0x14;
   be_val<int32_t> unk0x18;
   UNKNOWN(0x10);
   char volumeLabel[128];
   char volumeId[128];
   char devicePath[16];
   char mountPath[128];
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

} // namespace fsa

} // namespace ios

} // namespace kernel
