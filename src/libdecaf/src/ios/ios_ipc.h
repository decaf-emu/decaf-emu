#pragma once
#include "ios_enum.h"

#include <common/structsize.h>
#include <cstddef>
#include <libcpu/be2_struct.h>

namespace ios
{

#pragma pack(push, 1)

/**
 * Structure used for ioctlv arguments.
 */
struct IoctlVec
{
   //! Virtual address of buffer.
   be2_val<virt_addr> vaddr;

   //! Length of buffer.
   be2_val<uint32_t> len;

   //! Physical address of buffer.
   be2_val<phys_addr> paddr;
};
CHECK_OFFSET(IoctlVec, 0x00, vaddr);
CHECK_OFFSET(IoctlVec, 0x04, len);
CHECK_OFFSET(IoctlVec, 0x08, paddr);
CHECK_SIZE(IoctlVec, 0x0C);

struct IpcRequestArgsOpen
{
   be2_phys_ptr<const char> name;
   be2_val<uint32_t> nameLen;
   be2_val<OpenMode> mode;
};
CHECK_OFFSET(IpcRequestArgsOpen, 0x00, name);
CHECK_OFFSET(IpcRequestArgsOpen, 0x04, nameLen);
CHECK_OFFSET(IpcRequestArgsOpen, 0x08, mode);
CHECK_SIZE(IpcRequestArgsOpen, 0x0C);

struct IpcRequestArgsClose
{
   be2_val<uint32_t> unkArg0;
};
CHECK_OFFSET(IpcRequestArgsClose, 0x00, unkArg0);
CHECK_SIZE(IpcRequestArgsClose, 0x04);

struct IpcRequestArgsRead
{
   be2_phys_ptr<void> data;
   be2_val<uint32_t> length;
};
CHECK_OFFSET(IpcRequestArgsRead, 0x00, data);
CHECK_OFFSET(IpcRequestArgsRead, 0x04, length);
CHECK_SIZE(IpcRequestArgsRead, 0x08);

struct IpcRequestArgsWrite
{
   be2_phys_ptr<const void> data;
   be2_val<uint32_t> length;
};
CHECK_OFFSET(IpcRequestArgsWrite, 0x00, data);
CHECK_OFFSET(IpcRequestArgsWrite, 0x04, length);
CHECK_SIZE(IpcRequestArgsWrite, 0x08);

struct IpcRequestArgsSeek
{
   be2_val<uint32_t> offset;
   be2_val<SeekOrigin> origin;
};
CHECK_OFFSET(IpcRequestArgsSeek, 0x00, offset);
CHECK_OFFSET(IpcRequestArgsSeek, 0x04, origin);
CHECK_SIZE(IpcRequestArgsSeek, 0x08);

struct IpcRequestArgsIoctl
{
   be2_val<uint32_t> request;
   be2_phys_ptr<const void> inputBuffer;
   be2_val<uint32_t> inputLength;
   be2_phys_ptr<void> outputBuffer;
   be2_val<uint32_t> outputLength;
};
CHECK_OFFSET(IpcRequestArgsIoctl, 0x00, request);
CHECK_OFFSET(IpcRequestArgsIoctl, 0x04, inputBuffer);
CHECK_OFFSET(IpcRequestArgsIoctl, 0x08, inputLength);
CHECK_OFFSET(IpcRequestArgsIoctl, 0x0C, outputBuffer);
CHECK_OFFSET(IpcRequestArgsIoctl, 0x10, outputLength);
CHECK_SIZE(IpcRequestArgsIoctl, 0x14);

struct IpcRequestArgsIoctlv
{
   be2_val<uint32_t> request;
   be2_val<uint32_t> numVecIn;
   be2_val<uint32_t> numVecOut;
   be2_phys_ptr<IoctlVec> vecs;
};
CHECK_OFFSET(IpcRequestArgsIoctlv, 0x00, request);
CHECK_OFFSET(IpcRequestArgsIoctlv, 0x04, numVecIn);
CHECK_OFFSET(IpcRequestArgsIoctlv, 0x08, numVecOut);
CHECK_OFFSET(IpcRequestArgsIoctlv, 0x0C, vecs);
CHECK_SIZE(IpcRequestArgsIoctlv, 0x10);

struct IpcRequestArgsResume
{
   be2_val<uint32_t> unkArg0;
   be2_val<uint32_t> unkArg1;
};
CHECK_OFFSET(IpcRequestArgsResume, 0x00, unkArg0);
CHECK_OFFSET(IpcRequestArgsResume, 0x04, unkArg1);
CHECK_SIZE(IpcRequestArgsResume, 0x08);

struct IpcRequestArgsSuspend
{
   be2_val<uint32_t> unkArg0;
   be2_val<uint32_t> unkArg1;
};
CHECK_OFFSET(IpcRequestArgsSuspend, 0x00, unkArg0);
CHECK_OFFSET(IpcRequestArgsSuspend, 0x04, unkArg1);
CHECK_SIZE(IpcRequestArgsSuspend, 0x08);

struct IpcRequestArgsSvcMsg
{
   be2_val<uint32_t> unkArg0;
   be2_val<uint32_t> unkArg1;
   be2_val<uint32_t> unkArg2;
   be2_val<uint32_t> unkArg3;
};
CHECK_OFFSET(IpcRequestArgsSvcMsg, 0x00, unkArg0);
CHECK_OFFSET(IpcRequestArgsSvcMsg, 0x04, unkArg1);
CHECK_OFFSET(IpcRequestArgsSvcMsg, 0x08, unkArg2);
CHECK_OFFSET(IpcRequestArgsSvcMsg, 0x0C, unkArg3);
CHECK_SIZE(IpcRequestArgsSvcMsg, 0x10);

struct IpcRequestArgs
{
   union
   {
      be2_struct<IpcRequestArgsOpen> open;
      be2_struct<IpcRequestArgsClose> close;
      be2_struct<IpcRequestArgsRead> read;
      be2_struct<IpcRequestArgsWrite> write;
      be2_struct<IpcRequestArgsSeek> seek;
      be2_struct<IpcRequestArgsIoctl> ioctl;
      be2_struct<IpcRequestArgsIoctlv> ioctlv;
      be2_struct<IpcRequestArgsResume> resume;
      be2_struct<IpcRequestArgsSuspend> suspend;
      be2_struct<IpcRequestArgsSvcMsg> svcMsg;
      be2_array<uint32_t, 5> args;
   };
};
CHECK_OFFSET(IpcRequestArgs, 0x00, open);
CHECK_OFFSET(IpcRequestArgs, 0x00, read);
CHECK_OFFSET(IpcRequestArgs, 0x00, write);
CHECK_OFFSET(IpcRequestArgs, 0x00, seek);
CHECK_OFFSET(IpcRequestArgs, 0x00, ioctl);
CHECK_OFFSET(IpcRequestArgs, 0x00, ioctlv);
CHECK_OFFSET(IpcRequestArgs, 0x00, args);
CHECK_SIZE(IpcRequestArgs, 0x14);


struct IpcRequestOrigin
{
   //! Process ID the request originated from
   be2_val<uint32_t> clientPid;

   //! Title ID the request originated from
   be2_val<uint64_t> titleId;

   //! What is a gid???
   be2_val<uint32_t> gid;
};
CHECK_OFFSET(IpcRequestOrigin, 0x00, clientPid);
CHECK_OFFSET(IpcRequestOrigin, 0x04, titleId);
CHECK_OFFSET(IpcRequestOrigin, 0x0C, gid);
CHECK_SIZE(IpcRequestOrigin, 0x10);


/**
 * The actual data which is sent as an IPC request between IOSU (ARM) and
 * PowerPC cores.
 */
struct IpcRequest
{
   static constexpr auto ArgCount = 5;

   //! IOS command to execute
   be2_val<Command> command;

   //! IPC command result
   be2_val<Error> reply;

   //! Handle for the IOS resource
   be2_val<int32_t> handle;

   //! Flags, always 0
   be2_val<uint32_t> flags;

   //! CPU the request originated from
   be2_val<CpuID> cpuID;

   //! Process ID the request originated from
   be2_val<int32_t> clientPid;

   //! Title ID the request originated from
   be2_val<uint64_t> titleId;

   //! 'gid' ??
   be2_val<uint32_t> gid;

   //! IPC command args
   be2_struct<IpcRequestArgs> args;
};
CHECK_OFFSET(IpcRequest, 0x00, command);
CHECK_OFFSET(IpcRequest, 0x04, reply);
CHECK_OFFSET(IpcRequest, 0x08, handle);
CHECK_OFFSET(IpcRequest, 0x0C, flags);
CHECK_OFFSET(IpcRequest, 0x10, cpuID);
CHECK_OFFSET(IpcRequest, 0x14, clientPid);
CHECK_OFFSET(IpcRequest, 0x18, titleId);
CHECK_OFFSET(IpcRequest, 0x20, gid);
CHECK_OFFSET(IpcRequest, 0x24, args);
CHECK_SIZE(IpcRequest, 0x38);

#pragma pack(pop)

} // namespace ios
