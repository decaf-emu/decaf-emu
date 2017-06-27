#pragma once
#include "fsa_enum.h"
#include "fsa_types.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

namespace ios
{

namespace dev
{

namespace fsa
{

/**
 * \ingroup ios_dev_fsa
 * @{
 */

#pragma pack(push, 1)


/**
 * Request data for Command::ChangeDir
 */
struct FSARequestChangeDir
{
   char path[FSAMaxPathLength + 1];
};
CHECK_OFFSET(FSARequestChangeDir, 0x0, path);
CHECK_SIZE(FSARequestChangeDir, 0x280);


/**
 * Request data for Command::CloseDir
 */
struct FSARequestCloseDir
{
   be_val<FSADirHandle> handle;
};
CHECK_OFFSET(FSARequestCloseDir, 0x0, handle);
CHECK_SIZE(FSARequestCloseDir, 0x4);


/**
 * Request data for Command::CloseFile
 */
struct FSARequestCloseFile
{
   be_val<FSAFileHandle> handle;
};
CHECK_OFFSET(FSARequestCloseFile, 0x0, handle);
CHECK_SIZE(FSARequestCloseFile, 0x4);


/**
 * Request data for Command::FlushFile
 */
struct FSARequestFlushFile
{
   be_val<FSAFileHandle> handle;
};
CHECK_OFFSET(FSARequestFlushFile, 0x0, handle);
CHECK_SIZE(FSARequestFlushFile, 0x4);


/**
 * Request data for Command::FlushQuota
 */
struct FSARequestFlushQuota
{
   char path[FSAMaxPathLength + 1];
};
CHECK_OFFSET(FSARequestFlushQuota, 0x0, path);
CHECK_SIZE(FSARequestFlushQuota, 0x280);


/**
 * Request data for Command::GetInfoByQuery
 */
struct FSARequestGetInfoByQuery
{
   char path[FSAMaxPathLength + 1];
   be_val<FSAQueryInfoType> type;
};
CHECK_OFFSET(FSARequestGetInfoByQuery, 0x0, path);
CHECK_OFFSET(FSARequestGetInfoByQuery, 0x280, type);
CHECK_SIZE(FSARequestGetInfoByQuery, 0x284);


/**
 * Request data for Command::GetPosFile
 */
struct FSARequestGetPosFile
{
   be_val<FSAFileHandle> handle;
};
CHECK_OFFSET(FSARequestGetPosFile, 0x0, handle);
CHECK_SIZE(FSARequestGetPosFile, 0x4);


/**
 * Request data for Command::IsEof
 */
struct FSARequestIsEof
{
   be_val<FSAFileHandle> handle;
};
CHECK_OFFSET(FSARequestIsEof, 0x0, handle);
CHECK_SIZE(FSARequestIsEof, 0x4);


/**
 * Request data for Command::MakeDir
 */
struct FSARequestMakeDir
{
   char path[FSAMaxPathLength + 1];
   be_val<uint32_t> permission;
};
CHECK_OFFSET(FSARequestMakeDir, 0x0, path);
CHECK_OFFSET(FSARequestMakeDir, 0x280, permission);
CHECK_SIZE(FSARequestMakeDir, 0x284);


/**
 * Request data for Command::Mount
 */
struct FSARequestMount
{
   char path[FSAMaxPathLength + 1];
   char target[FSAMaxPathLength + 1];
   be_val<uint32_t> unk0x500;
   be_val<uint32_t> unk0x504;
   be_val<uint32_t> unk0x508;
};
CHECK_OFFSET(FSARequestMount, 0x0, path);
CHECK_OFFSET(FSARequestMount, 0x280, target);
CHECK_OFFSET(FSARequestMount, 0x500, unk0x500);
CHECK_OFFSET(FSARequestMount, 0x504, unk0x504);
CHECK_OFFSET(FSARequestMount, 0x508, unk0x508);
CHECK_SIZE(FSARequestMount, 0x50C);


/**
 * Request data for Command::OpenDir
 */
struct FSARequestOpenDir
{
   char path[FSAMaxPathLength + 1];
};
CHECK_OFFSET(FSARequestOpenDir, 0x0, path);
CHECK_SIZE(FSARequestOpenDir, 0x280);


/**
 * Request data for Command::OpenFile
 */
struct FSARequestOpenFile
{
   char path[FSAMaxPathLength + 1];
   char mode[0x10];
   be_val<uint32_t> unk0x290;
   be_val<uint32_t> unk0x294;
   be_val<uint32_t> unk0x298;
};
CHECK_OFFSET(FSARequestOpenFile, 0x0, path);
CHECK_OFFSET(FSARequestOpenFile, 0x280, mode);
CHECK_OFFSET(FSARequestOpenFile, 0x290, unk0x290);
CHECK_OFFSET(FSARequestOpenFile, 0x294, unk0x294);
CHECK_OFFSET(FSARequestOpenFile, 0x298, unk0x298);
CHECK_SIZE(FSARequestOpenFile, 0x29C);


/**
 * Request data for Command::ReadDir
 */
struct FSARequestReadDir
{
   be_val<FSADirHandle> handle;
};
CHECK_OFFSET(FSARequestReadDir, 0x0, handle);
CHECK_SIZE(FSARequestReadDir, 0x4);


/**
 * Request data for Command::ReadFile
 */
struct FSARequestReadFile
{
   be_ptr<uint8_t> buffer;
   be_val<uint32_t> size;
   be_val<uint32_t> count;
   be_val<FSAFilePosition> pos;
   be_val<FSAFileHandle> handle;
   be_val<FSAReadFlag> readFlags;
};
CHECK_OFFSET(FSARequestReadFile, 0x00, buffer);
CHECK_OFFSET(FSARequestReadFile, 0x04, size);
CHECK_OFFSET(FSARequestReadFile, 0x08, count);
CHECK_OFFSET(FSARequestReadFile, 0x0C, pos);
CHECK_OFFSET(FSARequestReadFile, 0x10, handle);
CHECK_OFFSET(FSARequestReadFile, 0x14, readFlags);
CHECK_SIZE(FSARequestReadFile, 0x18);


/**
 * Request data for Command::Remove
 */
struct FSARequestRemove
{
   char path[FSAMaxPathLength + 1];
};
CHECK_OFFSET(FSARequestRemove, 0x0, path);
CHECK_SIZE(FSARequestRemove, 0x280);


/**
 * Request data for Command::Rename
 */
struct FSARequestRename
{
   char oldPath[FSAMaxPathLength + 1];
   char newPath[FSAMaxPathLength + 1];
};
CHECK_OFFSET(FSARequestRename, 0x0, oldPath);
CHECK_OFFSET(FSARequestRename, 0x280, newPath);
CHECK_SIZE(FSARequestRename, 0x500);


/**
 * Request data for Command::RewindDir
 */
struct FSARequestRewindDir
{
   be_val<FSADirHandle> handle;
};
CHECK_OFFSET(FSARequestRewindDir, 0x0, handle);
CHECK_SIZE(FSARequestRewindDir, 0x4);


/**
 * Request data for Command::SetPosFile
 */
struct FSARequestSetPosFile
{
   be_val<FSAFileHandle> handle;
   be_val<FSAFilePosition> pos;
};
CHECK_OFFSET(FSARequestSetPosFile, 0x0, handle);
CHECK_OFFSET(FSARequestSetPosFile, 0x4, pos);
CHECK_SIZE(FSARequestSetPosFile, 0x8);


/**
 * Request data for Command::StatFile
 */
struct FSARequestStatFile
{
   be_val<FSAFileHandle> handle;
};
CHECK_OFFSET(FSARequestStatFile, 0x0, handle);
CHECK_SIZE(FSARequestStatFile, 0x4);


/**
 * Request data for Command::TruncateFile
 */
struct FSARequestTruncateFile
{
   be_val<FSAFileHandle> handle;
};
CHECK_OFFSET(FSARequestTruncateFile, 0x0, handle);
CHECK_SIZE(FSARequestTruncateFile, 0x4);


/**
 * Request data for Command::Unmount
 */
struct FSARequestUnmount
{
   char path[FSAMaxPathLength + 1];
   be_val<uint32_t> unk0x280;
};
CHECK_OFFSET(FSARequestUnmount, 0x0, path);
CHECK_OFFSET(FSARequestUnmount, 0x280, unk0x280);
CHECK_SIZE(FSARequestUnmount, 0x284);


/**
 * Request data for Command::WriteFile
 */
struct FSARequestWriteFile
{
   be_ptr<const uint8_t> buffer;
   be_val<uint32_t> size;
   be_val<uint32_t> count;
   be_val<FSAFilePosition> pos;
   be_val<FSAFileHandle> handle;
   be_val<FSAWriteFlag> writeFlags;
};
CHECK_OFFSET(FSARequestWriteFile, 0x00, buffer);
CHECK_OFFSET(FSARequestWriteFile, 0x04, size);
CHECK_OFFSET(FSARequestWriteFile, 0x08, count);
CHECK_OFFSET(FSARequestWriteFile, 0x0C, pos);
CHECK_OFFSET(FSARequestWriteFile, 0x10, handle);
CHECK_OFFSET(FSARequestWriteFile, 0x14, writeFlags);
CHECK_SIZE(FSARequestWriteFile, 0x18);


/**
 * IPC request data for FSA device.
 */
struct FSARequest
{
   be_val<FSAStatus> emulatedError;

   union
   {
      FSARequestChangeDir changeDir;
      FSARequestCloseDir closeDir;
      FSARequestCloseFile closeFile;
      FSARequestFlushFile flushFile;
      FSARequestFlushQuota flushQuota;
      FSARequestGetInfoByQuery getInfoByQuery;
      FSARequestGetPosFile getPosFile;
      FSARequestIsEof isEof;
      FSARequestMakeDir makeDir;
      FSARequestMount mount;
      FSARequestOpenDir openDir;
      FSARequestOpenFile openFile;
      FSARequestReadDir readDir;
      FSARequestReadFile readFile;
      FSARequestRemove remove;
      FSARequestRename rename;
      FSARequestRewindDir rewindDir;
      FSARequestSetPosFile setPosFile;
      FSARequestStatFile statFile;
      FSARequestTruncateFile truncateFile;
      FSARequestUnmount unmount;
      FSARequestWriteFile writeFile;
      UNKNOWN(0x51C);
   };
};
CHECK_OFFSET(FSARequest, 0x00, emulatedError);
CHECK_SIZE(FSARequest, 0x520);

#pragma pack(pop)

/** @} */

} // namespace fsa

} // namespace dev

} // namespace ios
