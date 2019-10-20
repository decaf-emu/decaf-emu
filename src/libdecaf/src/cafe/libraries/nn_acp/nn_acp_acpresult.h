#pragma once
#include "nn/nn_result.h"

#include <cstdint>

namespace cafe::nn_acp
{

enum class ACPResult : int32_t
{
   Success                 = 0,

   Invalid                 = -200,
   InvalidParameter        = -201,
   InvalidFile             = -202,
   InvalidXmlFile          = -203,
   FileAccessMode          = -204,
   InvalidNetworkTime      = -205,

   NotFound                = -500,
   FileNotFound            = -501,
   DirNotFound             = -502,
   DeviceNotFound          = -503,
   TitleNotFound           = -504,
   ApplicationNotFound     = -505,
   SystemConfigNotFound    = -506,
   XmlItemNotFound         = -507,

   AlreadyExists           = -600,
   FileAlreadyExists       = -601,
   DirAlreadyExists        = -602,

   AlreadyDone             = -700,

   Authentication          = -1000,
   InvalidRegion           = -1001,
   RestrictedRating        = -1002,
   NotPresentRating        = -1003,
   PendingRating           = -1004,
   NetSettingRequired      = -1005,
   NetAccountRequired      = -1006,
   NetAccountError         = -1007,
   BrowserRequired         = -1008,
   OlvRequired             = -1009,
   PincodeRequired         = -1010,
   IncorrectPincode        = -1011,
   InvalidLogo             = -1012,
   DemoExpiredNumber       = -1013,
   DrcRequired             = -1014,

   NoPermission            = -1100,
   NoFilePermission        = -1101,
   NoDirPermission         = -1102,

   Busy                    = -1300,
   UsbStorageNotReady      = -1301,

   Cancelled               = -1400,

   Resource                = -1500,
   DeviceFull              = -1501,
   JournalFull             = -1502,
   SystemMemory            = -1503,
   FsResource              = -1504,
   IpcResource             = -1505,

   NotInitialised          = -1600,

   AccountError            = -1700,

   Unsupported             = -1800,

   DataCorrupted           = -2000,
   Device                  = -2001,
   SlcDataCorrupted        = -2002,
   MlcDataCorrupted        = -2003,
   UsbDataCorrupted        = -2004,

   Media                   = -2100,
   MediaNotReady           = -2101,
   MediaBroken             = -2102,
   OddMediaNotReady        = -2103,
   OddMediaBroken          = -2104,
   UsbMediaNotReady        = -2105,
   UsbMediaBroken          = -2106,
   MediaWriteProtected     = -2107,
   UsbWriteProtected       = -2108,

   Mii                     = -2200,
   EncryptionError         = -2201,

   GenericError            = -4096,
};

ACPResult
ACPConvertToACPResult(nn::Result result,
                      const char *funcName,
                      int32_t lineNo);

} // namespace cafe::nn_acp
