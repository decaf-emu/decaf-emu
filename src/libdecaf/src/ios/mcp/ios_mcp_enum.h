#ifndef IOS_MCP_ENUM_H
#define IOS_MCP_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(ios)

ENUM_NAMESPACE_ENTER(mcp)

ENUM_BEG(MainThreadCommand, uint32_t)
   ENUM_VALUE(SysEvent,                   0x101)
ENUM_END(MainThreadCommand)

ENUM_BEG(MCPAppType, uint32_t)
   ENUM_VALUE(Unk0x0800000E,        0x0800000E)
ENUM_END(MCPAppType)

ENUM_BEG(MCPCommand, uint32_t)
   ENUM_VALUE(GetEnvironmentVariable,     0x20)
   ENUM_VALUE(GetSysProdSettings,         0x40)
   ENUM_VALUE(SetSysProdSettings,         0x41)
   ENUM_VALUE(GetOwnTitleInfo,            0x4C)
   ENUM_VALUE(TitleCount,                 0x4D)
   ENUM_VALUE(CloseTitle,                 0x50)
   ENUM_VALUE(SwitchTitle,                0x51)
   ENUM_VALUE(PrepareTitle0x52,           0x52)
   ENUM_VALUE(LoadFile,                   0x53)
   ENUM_VALUE(GetFileLength,              0x57)
   ENUM_VALUE(SearchTitleList,            0x58)
   ENUM_VALUE(PrepareTitle0x59,           0x59)
   ENUM_VALUE(GetLaunchParameters,        0x5A)
   ENUM_VALUE(GetTitleId,                 0x5B)
ENUM_END(MCPCommand)

ENUM_BEG(MCPCountryCode, uint32_t)
   ENUM_VALUE(USA,                  0x31)
   ENUM_VALUE(UnitedKingdom,        0x63)
ENUM_END(MCPCountryCode)

ENUM_BEG(MCPError, int32_t)
   ENUM_VALUE(OK,                   0)
   ENUM_VALUE(Invalid,              -0x40001)
   ENUM_VALUE(System,               -0x40002)
   ENUM_VALUE(Alloc,                -0x40003)
   ENUM_VALUE(Opcode,               -0x40004)
   ENUM_VALUE(InvalidParam,         -0x40005)
   ENUM_VALUE(InvalidType,          -0x40006)
   ENUM_VALUE(Unsupported,          -0x40007)
   ENUM_VALUE(NonLeafNode,          -0x4000A)
   ENUM_VALUE(KeyNotFound,          -0x4000B)
   ENUM_VALUE(Modify,               -0x4000C)
   ENUM_VALUE(StringTooLong,        -0x4000D)
   ENUM_VALUE(RootKeysDiffer,       -0x4000E)
   ENUM_VALUE(InvalidLocation,      -0x4000F)
   ENUM_VALUE(BadComment,           -0x40010)
   ENUM_VALUE(ReadAccess,           -0x40011)
   ENUM_VALUE(WriteAccess,          -0x40012)
   ENUM_VALUE(CreateAccess,         -0x40013)
   ENUM_VALUE(FileSysName,          -0x40014)
   ENUM_VALUE(FileSysInit,          -0x40015)
   ENUM_VALUE(FileSysMount,         -0x40016)
   ENUM_VALUE(FileOpen,             -0x40017)
   ENUM_VALUE(FileStat,             -0x40018)
   ENUM_VALUE(FileRead,             -0x40019)
   ENUM_VALUE(FileWrite,            -0x4001A)
   ENUM_VALUE(FileTooBig,           -0x4001B)
   ENUM_VALUE(FileRemove,           -0x4001C)
   ENUM_VALUE(FileRename,           -0x4001D)
   ENUM_VALUE(FileClose,            -0x4001E)
   ENUM_VALUE(FileSeek,             -0x4001F)
   ENUM_VALUE(MalformedXML,         -0x40022)
   ENUM_VALUE(Version,              -0x40023)
   ENUM_VALUE(NoIpcBuffers,         -0x40024)
   ENUM_VALUE(FileLockNeeded,       -0x40026)
   ENUM_VALUE(SysProt,              -0x40032)
   ENUM_VALUE(AlreadyOpen,          -0x40054)
   ENUM_VALUE(StorageFull,          -0x40055)
   ENUM_VALUE(WriteProtected,       -0x40056)
   ENUM_VALUE(DataCorrupted,        -0x40057)
   ENUM_VALUE(KernelErrorBase,      -0x403E8)
ENUM_END(MCPError)

ENUM_BEG(MCPFileType, uint32_t)
   //! Load from the process's code directory (process title id)/code/%s
   ENUM_VALUE(ProcessCode,          0x00)

   //! Load from the CafeOS directory (00050010-1000400A)/code/%s
   ENUM_VALUE(CafeOS,               0x01)

   //! Load from the shared data content directory (0005001B-10042400)/content/%s
   ENUM_VALUE(SharedDataContent,    0x02)

   //! Load from the shared data code directory (0005001B-10042400)/code/%s
   ENUM_VALUE(SharedDataCode,       0x03)
ENUM_END(MCPFileType)

ENUM_BEG(MCPResourcePermissions, uint32_t)
   ENUM_VALUE(AddOnContent,         0x80)
   ENUM_VALUE(SciErrorLog,          0x200)
ENUM_END(MCPResourcePermissions)

ENUM_BEG(MCPRegion, uint32_t)
   ENUM_VALUE(Japan,                0x01)
   ENUM_VALUE(USA,                  0x02)
   ENUM_VALUE(Europe,               0x04)
   ENUM_VALUE(Unknown8,             0x08)
   ENUM_VALUE(China,                0x10)
   ENUM_VALUE(Korea,                0x20)
   ENUM_VALUE(Taiwan,               0x40)
ENUM_END(MCPRegion)

FLAGS_BEG(MCPTitleListSearchFlags, uint32_t)
   FLAGS_VALUE(None,                0)
   FLAGS_VALUE(TitleId,             1 << 0)
   FLAGS_VALUE(AppType,             1 << 2)
   FLAGS_VALUE(IndexedDevice,       1 << 6)
   FLAGS_VALUE(Unk0x60,             1 << 8)
   FLAGS_VALUE(Path,                1 << 9)
   FLAGS_VALUE(UniqueId,            1 << 10)
FLAGS_END(MCPTitleListSearchFlags)

ENUM_BEG(PMCommand, uint32_t)
   ENUM_VALUE(GetResourceManagerId,       0xE0)
   ENUM_VALUE(RegisterResourceManager,    0xE1)
ENUM_END(PMCommand)

ENUM_BEG(PPCAppCommand, uint32_t)
   ENUM_VALUE(StartupEvent,               0xB0)
   ENUM_VALUE(PowerOff,                   0xB2)
   ENUM_VALUE(UnrecoverableError,         0xB3)
ENUM_END(PPCAppCommand)

ENUM_BEG(ResourceManagerCommand, int32_t)
   ENUM_VALUE(Timeout,                    -32)  // ios::Error::Timeout
   ENUM_VALUE(ResumeReply,                8)    // ios::Command::Reply
   ENUM_VALUE(Register,                   16)
ENUM_END(ResourceManagerCommand)

ENUM_BEG(ResourceManagerRegistrationState, uint32_t)
   ENUM_VALUE(Invalid,                    0)
   ENUM_VALUE(Registered,                 1)
   ENUM_VALUE(NotRegistered,              2)
   ENUM_VALUE(Resumed,                    3)
   ENUM_VALUE(Suspended,                  4)
   ENUM_VALUE(Pending,                    5)
   ENUM_VALUE(Failed,                     6)
ENUM_END(ResourceManagerRegistrationState)

ENUM_BEG(SystemFileSys, uint32_t)
   ENUM_VALUE(Nand,                       1)
   ENUM_VALUE(Pcfs,                       2)
   ENUM_VALUE(SdCard,                     3)
ENUM_END(SystemFileSys)

ENUM_NAMESPACE_EXIT(mcp)

ENUM_NAMESPACE_EXIT(ios)

#include <common/enum_end.inl>

#endif // ifdef IOS_MCP_ENUM_H
