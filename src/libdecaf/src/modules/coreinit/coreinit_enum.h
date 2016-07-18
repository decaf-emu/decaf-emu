#ifndef COREINIT_ENUM_H
#define COREINIT_ENUM_H

#include "common/enum_start.h"

ENUM_NAMESPACE_BEG(coreinit)

ENUM_BEG(HardwareVersion, uint32_t)
   ENUM_VALUE(UNKNOWN,                       0x00000000)

   // vWii Hardware Versions
   ENUM_VALUE(HOLLYWOOD_ENG_SAMPLE_1,        0x00000001)
   ENUM_VALUE(HOLLYWOOD_ENG_SAMPLE_2,        0x10000001)
   ENUM_VALUE(HOLLYWOOD_PROD_FOR_WII,        0x10100001)
   ENUM_VALUE(HOLLYWOOD_CORTADO,             0x10100008)
   ENUM_VALUE(HOLLYWOOD_CORTADO_ESPRESSO,    0x1010000C)
   ENUM_VALUE(BOLLYWOOD,                     0x20000001)
   ENUM_VALUE(BOLLYWOOD_PROD_FOR_WII,        0x20100001)

   // WiiU Hardware Versions
   ENUM_VALUE(LATTE_A11_EV,                  0x21100010)
   ENUM_VALUE(LATTE_A11_CAT,                 0x21100020)
   ENUM_VALUE(LATTE_A12_EV,                  0x21200010)
   ENUM_VALUE(LATTE_A12_CAT,                 0x21200020)
   ENUM_VALUE(LATTE_A2X_EV,                  0x22100010)
   ENUM_VALUE(LATTE_A2X_CAT,                 0x22100020)
   ENUM_VALUE(LATTE_A3X_EV,                  0x23100010)
   ENUM_VALUE(LATTE_A3X_CAT,                 0x23100020)
   ENUM_VALUE(LATTE_A3X_CAFE,                0x23100028)
   ENUM_VALUE(LATTE_A4X_EV,                  0x24100010)
   ENUM_VALUE(LATTE_A4X_CAT,                 0x24100020)
   ENUM_VALUE(LATTE_A4X_CAFE,                0x24100028)
   ENUM_VALUE(LATTE_A5X_EV,                  0x25100010)
   ENUM_VALUE(LATTE_A5X_EV_Y,                0x25100011)
   ENUM_VALUE(LATTE_A5X_CAT,                 0x25100020)
   ENUM_VALUE(LATTE_A5X_CAFE,                0x25100028)
   ENUM_VALUE(LATTE_B1X_EV,                  0x26100010)
   ENUM_VALUE(LATTE_B1X_EV_Y,                0x26100011)
   ENUM_VALUE(LATTE_B1X_CAT,                 0x26100020)
   ENUM_VALUE(LATTE_B1X_CAFE,                0x26100028)
ENUM_END(HardwareVersion)

ENUM_BEG(OSAlarmState, uint32_t)
   ENUM_VALUE(None,                 0)
   ENUM_VALUE(Set,                  1)
   ENUM_VALUE(Expired,              2)
ENUM_END(OSAlarmState)

ENUM_BEG(OSEventMode, uint32_t)
   //! A manual event will only reset when OSResetEvent is called.
   ENUM_VALUE(ManualReset,          0)

   //! An auto event will reset everytime a thread is woken.
   ENUM_VALUE(AutoReset,            1)
ENUM_END(OSEventMode)

ENUM_BEG(OSExceptionType, uint32_t)
   ENUM_VALUE(SystemReset,          0)
   ENUM_VALUE(MachineCheck,         1)
   ENUM_VALUE(DSI,                  2)
   ENUM_VALUE(ISI,                  3)
   ENUM_VALUE(ExternalInterrupt,    4)
   ENUM_VALUE(Alignment,            5)
   ENUM_VALUE(Program,              6)
   ENUM_VALUE(FloatingPoint,        7)
   ENUM_VALUE(Decrementer,          8)
   ENUM_VALUE(SystemCall,           9)
   ENUM_VALUE(Trace,                10)
   ENUM_VALUE(PerformanceMonitor,   11)
   ENUM_VALUE(Breakpoint,           12)
   ENUM_VALUE(SystemInterrupt,      13)
   ENUM_VALUE(ICI,                  14)
   ENUM_VALUE(Max,                  15)
ENUM_END(OSExceptionType)

ENUM_BEG(OSMemoryType, uint32_t)
   ENUM_VALUE(MEM1,                 1)
   ENUM_VALUE(MEM2,                 2)
ENUM_END(OSMemoryType)

ENUM_BEG(OSMessageFlags, uint32_t)
   ENUM_VALUE(None,                 0)
   ENUM_VALUE(Blocking,             1 << 0)
   ENUM_VALUE(HighPriority,         1 << 1)
ENUM_END(OSMessageFlags)

ENUM_BEG(OSSharedDataType, uint32_t)
   ENUM_VALUE(FontChinese,          0)
   ENUM_VALUE(FontKorean,           1)
   ENUM_VALUE(FontStandard,         2)
   ENUM_VALUE(FontTaiwanese,        3)
ENUM_END(OSSharedDataType)

ENUM_BEG(OSThreadAttributes, uint8_t)
   //! Allow the thread to run on CPU0.
   ENUM_VALUE(AffinityCPU0,         1 << 0)

   //! Allow the thread to run on CPU1.
   ENUM_VALUE(AffinityCPU1,         1 << 1)

   //! Allow the thread to run on CPU2.
   ENUM_VALUE(AffinityCPU2,         1 << 2)

   //! Allow the thread to run any CPU.
   ENUM_VALUE(AffinityAny,          (1 << 0) | (1 << 1) | (1 << 2))

   //! Start the thread detached.
   ENUM_VALUE(Detached,             1 << 3)

   //! Enables tracking of stack usage.
   ENUM_VALUE(StackUsage,           1 << 5)
ENUM_END(OSThreadAttributes)

ENUM_BEG(OSThreadRequest, uint32_t)
   ENUM_VALUE(None,                 0)
   ENUM_VALUE(Suspend,              1)
   ENUM_VALUE(Cancel,               2)
ENUM_END(OSThreadRequest)

ENUM_BEG(OSThreadState, uint8_t)
   ENUM_VALUE(None,                 0)

   //! Thread is ready to run
   ENUM_VALUE(Ready,                1 << 0)

   //! Thread is running
   ENUM_VALUE(Running,              1 << 1)

   //! Thread is waiting, i.e. on a mutex
   ENUM_VALUE(Waiting,              1 << 2)

   //! Thread is about to terminate
   ENUM_VALUE(Moribund,             1 << 3)
ENUM_END(OSThreadState)

ENUM_BEG(FSStatus, int32_t)
   ENUM_VALUE(OK,                   0)
   ENUM_VALUE(Cancelled,            -1)
   ENUM_VALUE(End,                  -2)
   ENUM_VALUE(Max,                  -3)
   ENUM_VALUE(AlreadyOpen,          -4)
   ENUM_VALUE(Exists,               -5)
   ENUM_VALUE(NotFound,             -6)
   ENUM_VALUE(NotFile,              -7)
   ENUM_VALUE(NotDir,               -8)
   ENUM_VALUE(AccessError,          -9)
   ENUM_VALUE(PermissionError,      -10)
   ENUM_VALUE(FileTooBig,           -11)
   ENUM_VALUE(StorageFull,          -12)
   ENUM_VALUE(JournalFull,          -13)
   ENUM_VALUE(UnsupportedCmd,       -14)
   ENUM_VALUE(MediaNotReady,        -15)
   ENUM_VALUE(MediaError,           -17)
   ENUM_VALUE(Corrupted,            -18)
   ENUM_VALUE(FatalError,           -0x400)
ENUM_END(FSStatus)

ENUM_BEG(FSError, int32_t)
   ENUM_VALUE(NotInit,              -0x30001)
   ENUM_VALUE(Busy,                 -0x30002)
   ENUM_VALUE(Cancelled,            -0x30003)
   ENUM_VALUE(EndOfDir,             -0x30004)
   ENUM_VALUE(EndOfFile,            -0x30005)
   ENUM_VALUE(MaxMountpoints,       -0x30010)
   ENUM_VALUE(MaxVolumes,           -0x30011)
   ENUM_VALUE(MaxClients,           -0x30012)
   ENUM_VALUE(MaxFiles,             -0x30013)
   ENUM_VALUE(MaxDirs,              -0x30014)
   ENUM_VALUE(AlreadyOpen,          -0x30015)
   ENUM_VALUE(AlreadyExists,        -0x30016)
   ENUM_VALUE(NotFound,             -0x30017)
   ENUM_VALUE(NotEmpty,             -0x30018)
   ENUM_VALUE(AccessError,          -0x30019)
   ENUM_VALUE(PermissionError,      -0x3001a)
   ENUM_VALUE(DataCorrupted,        -0x3001b)
   ENUM_VALUE(StorageFull,          -0x3001c)
   ENUM_VALUE(JournalFull,          -0x3001d)
   ENUM_VALUE(UnavailableCommand,   -0x3001f)
   ENUM_VALUE(UnsupportedCommand,   -0x30020)
   ENUM_VALUE(InvalidParam,         -0x30021)
   ENUM_VALUE(InvalidPath,          -0x30022)
   ENUM_VALUE(InvalidBuffer,        -0x30023)
   ENUM_VALUE(InvalidAlignment,     -0x30024)
   ENUM_VALUE(InvalidClientHandle,  -0x30025)
   ENUM_VALUE(InvalidFileHandle,    -0x30026)
   ENUM_VALUE(InvalidDirHandle,     -0x30027)
   ENUM_VALUE(NotFile,              -0x30028)
   ENUM_VALUE(NotDir,               -0x30029)
   ENUM_VALUE(FileTooBig,           -0x3002a)
   ENUM_VALUE(OutOfRange,           -0x3002b)
   ENUM_VALUE(OutOfResources,       -0x3002c)
   ENUM_VALUE(MediaNotReady,        -0x30030)
   ENUM_VALUE(MediaError,           -0x30031)
   ENUM_VALUE(WriteProtected,       -0x30032)
   ENUM_VALUE(InvalidMedia,         -0x30033)
ENUM_END(FSError)

ENUM_BEG(FSVolumeState, uint32_t)
   ENUM_VALUE(Initial,              0)
   ENUM_VALUE(Ready,                1)
   ENUM_VALUE(NoMedia,              2)
   ENUM_VALUE(InvalidMedia,         3)
   ENUM_VALUE(DirtyMedia,           4)
   ENUM_VALUE(WrongMedia,           5)
   ENUM_VALUE(MediaError,           6)
   ENUM_VALUE(DataCorrupted,        7)
   ENUM_VALUE(WriteProtected,       8)
   ENUM_VALUE(JournalFull,          9)
   ENUM_VALUE(Fatal,                10)
   ENUM_VALUE(Invalid,              11)
ENUM_END(FSVolumeState)

ENUM_BEG(MEMBaseHeapType, uint32_t)
   ENUM_VALUE(MEM1,                 0)
   ENUM_VALUE(MEM2,                 1)
   ENUM_VALUE(FG,                   8)
   ENUM_VALUE(Max,                  9)
   ENUM_VALUE(Invalid,              10)
ENUM_END(MEMBaseHeapType)

ENUM_BEG(MEMExpHeapMode, uint32_t)
   ENUM_VALUE(FirstFree,            0)
   ENUM_VALUE(NearestSize,          1)
ENUM_END(MEMExpHeapMode)

ENUM_BEG(MEMExpHeapDirection, uint32_t)
   ENUM_VALUE(FromTop,              0)
   ENUM_VALUE(FromBottom,           1)
ENUM_END(MEMExpHeapDirection)

ENUM_BEG(MEMFrameHeapFreeMode, uint32_t)
   ENUM_VALUE(Head,                 1 << 0)
   ENUM_VALUE(Tail,                 1 << 1)
ENUM_END(MEMFrameHeapFreeMode)

ENUM_BEG(MEMHeapTag, uint32_t)
   ENUM_VALUE(ExpandedHeap,         0x45585048)
   ENUM_VALUE(FrameHeap,            0x46524D48)
   ENUM_VALUE(UnitHeap,             0x554E5448)
   ENUM_VALUE(UserHeap,             0x55535248)
   ENUM_VALUE(BlockHeap,            0x424C4B48)
ENUM_END(MEMHeapTag)

ENUM_BEG(MEMHeapFillType, uint32_t)
   ENUM_VALUE(Unused,               0x0)
   ENUM_VALUE(Allocated,            0x1)
   ENUM_VALUE(Freed,                0x2)
   ENUM_VALUE(Max,                  0x3)
ENUM_END(MEMHeapFillType)

ENUM_BEG(MEMHeapFlags, uint32_t)
   ENUM_VALUE(ZeroAllocated,        1 << 0)
   ENUM_VALUE(DebugMode,            1 << 1)
   ENUM_VALUE(UseLock,              1 << 2)
ENUM_END(MEMHeapFlags)

ENUM_BEG(MEMProtectMode, uint32_t)
   ENUM_VALUE(ReadOnly,             1 << 0)
   ENUM_VALUE(ReadWrite,            1 << 1)
ENUM_END(MEMProtectMode)

ENUM_BEG(MPTaskQueueState, uint32_t)
   ENUM_VALUE(Initialised,          1 << 0)
   ENUM_VALUE(Ready,                1 << 1)
   ENUM_VALUE(Stopping,             1 << 2)
   ENUM_VALUE(Stopped,              1 << 3)
   ENUM_VALUE(Finished,             1 << 4)
ENUM_END(MPTaskQueueState)

ENUM_BEG(MPTaskState, uint32_t)
   ENUM_VALUE(Initialised,          1 << 0)
   ENUM_VALUE(Ready,                1 << 1)
   ENUM_VALUE(Running,              1 << 2)
   ENUM_VALUE(Finished,             1 << 3)
ENUM_END(MPTaskState)

ENUM_BEG(SCICountryCode, uint32_t)
   ENUM_VALUE(USA,                  0x31)
   ENUM_VALUE(UnitedKingdom,        0x63)
ENUM_END(SCICountryCode)

ENUM_BEG(SCILanguage, uint32_t)
   ENUM_VALUE(English,              0x01)
ENUM_END(SCILanguage)

ENUM_BEG(SCIRegion, uint8_t)
   ENUM_VALUE(JAP,                  0x01)
   ENUM_VALUE(US,                   0x02)
   ENUM_VALUE(EUR,                  0x04)
ENUM_END(SCIRegion)

ENUM_BEG(UCDataType, uint32_t)
   ENUM_VALUE(Uint8,                0x01)
   ENUM_VALUE(Uint16,               0x02)
   ENUM_VALUE(Uint32,               0x03)
   ENUM_VALUE(Blob,                 0x07)
   ENUM_VALUE(Group,                0x08)
ENUM_END(UCDataType)

ENUM_NAMESPACE_END(coreinit)

#include "common/enum_end.h"

#endif // ifdef COREINIT_ENUM_H
