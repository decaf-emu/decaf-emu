#include "nn_acp_acpresult.h"
#include "nn/acp/nn_acp_result.h"
#include "nn/ipc/nn_ipc_result.h"

#include "cafe/libraries/coreinit/coreinit_cosreport.h"
#include "cafe/libraries/coreinit/coreinit_osreport.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/cafe_stackobject.h"

using namespace cafe::coreinit;
using namespace nn::acp;

namespace cafe::nn_acp
{

static void
ACPSendCOSFatalError(coreinit::OSFatalErrorMessageType type,
                     uint32_t errorCode,
                     nn::Result result,
                     const char *funcName,
                     int32_t lineNo)
{
   internal::COSWarn(COSReportModule::Unknown1,
      fmt::format("ACP: ##### FATAL ERROR at {} ({})#####\n",
         funcName, result.code()));

   if (OSGetUPID() == cafe::kernel::UniqueProcessId::ErrorDisplay) {
      internal::COSWarn(COSReportModule::Unknown1,
         fmt::format("ACP: Skip to call OSSendFatalError from {} (UPID={}).\n",
            funcName, cafe::kernel::UniqueProcessId::ErrorDisplay));
      return;
   }

   StackObject<OSFatalError> fatalError;
   fatalError->messageType = type;
   fatalError->errorCode = errorCode;
   fatalError->internalErrorCode = static_cast<uint32_t>(result);
   if (funcName) {
      OSSendFatalError(fatalError, make_stack_string(funcName), lineNo);
   } else {
      OSSendFatalError(fatalError, make_stack_string("ACPSendCOSFatalError"), 103);
   }
}

ACPResult
ACPConvertToACPResult(nn::Result result,
                      const char *funcName,
                      int32_t lineNo)
{
   if (result.ok()) {
       return ACPResult::Success;
   }

   if (result == ResultInvalidParameter) {
      return ACPResult::InvalidParameter;
   } else if (result == ResultInvalidFile) {
      return ACPResult::InvalidFile;
   } else if (result == ResultInvalidXmlFile) {
      return ACPResult::InvalidXmlFile;
   } else if (result == ResultFileAccessMode) {
      return ACPResult::FileAccessMode;
   } else if (result == ResultInvalidNetworkTime) {
      return ACPResult::InvalidNetworkTime;
   } else if (result == ResultInvalid) {
      return ACPResult::Invalid;
   }

   if (result == ResultFileNotFound) {
      return ACPResult::FileNotFound;
   } else if (result == ResultDirNotFound) {
      return ACPResult::DirNotFound;
   } else if (result == ResultDeviceNotFound) {
      return ACPResult::DeviceNotFound;
   } else if (result == ResultTitleNotFound) {
      return ACPResult::TitleNotFound;
   } else if (result == ResultApplicationNotFound) {
      return ACPResult::ApplicationNotFound;
   } else if (result == ResultSystemConfigNotFound) {
      return ACPResult::SystemConfigNotFound;
   } else if (result == ResultXmlItemNotFound) {
      return ACPResult::XmlItemNotFound;
   } else if (result == ResultNotFound) {
      return ACPResult::NotFound;
   }

   if (result == ResultFileAlreadyExists) {
      return ACPResult::FileAlreadyExists;
   } else if (result == ResultDirAlreadyExists) {
      return ACPResult::DirAlreadyExists;
   } else if (result == ResultAlreadyExists) {
      return ACPResult::AlreadyExists;
   }

   if (result == ResultAlreadyDone) {
      return ACPResult::AlreadyDone;
   }

   if (result == ResultInvalidRegion) {
      return ACPResult::InvalidRegion;
   } else if (result == ResultRestrictedRating) {
      return ACPResult::RestrictedRating;
   } else if (result == ResultNotPresentRating) {
      return ACPResult::NotPresentRating;
   } else if (result == ResultPendingRating) {
      return ACPResult::PendingRating;
   } else if (result == ResultNetSettingRequired) {
      return ACPResult::NetSettingRequired;
   } else if (result == ResultNetAccountRequired) {
      return ACPResult::NetAccountRequired;
   } else if (result == ResultNetAccountError) {
      return ACPResult::NetAccountError;
   } else if (result == ResultBrowserRequired) {
      return ACPResult::BrowserRequired;
   } else if (result == ResultOlvRequired) {
      return ACPResult::OlvRequired;
   } else if (result == ResultPincodeRequired) {
      return ACPResult::PincodeRequired;
   } else if (result == ResultIncorrectPincode) {
      return ACPResult::IncorrectPincode;
   } else if (result == ResultInvalidLogo) {
      return ACPResult::InvalidLogo;
   } else if (result == ResultDemoExpiredNumber) {
      return ACPResult::DemoExpiredNumber;
   } else if (result == ResultDrcRequired) {
      return ACPResult::DrcRequired;
   } else if (result == ResultAuthentication) {
      return ACPResult::Authentication;
   }

   if (result == ResultNoFilePermission) {
      return ACPResult::NoFilePermission;
   } else if (result == ResultNoDirPermission) {
      return ACPResult::NoDirPermission;
   } else if (result == ResultNoPermission) {
      return ACPResult::NoPermission;
   }

   if (result == ResultUsbStorageNotReady) {
      return ACPResult::UsbStorageNotReady;
   } else if (result == ResultBusy) {
      return ACPResult::Busy;
   }

   if (result == ResultCancelled) {
      return ACPResult::Cancelled;
   }

   if (result == ResultDeviceFull) {
      return ACPResult::DeviceFull;
   } else if (result == ResultJournalFull) {
      return ACPResult::JournalFull;
   } else if (result == ResultSystemMemory) {
      return ACPResult::SystemMemory;
   } else if (result == ResultFsResource) {
      return ACPResult::FsResource;
   } else if (result == ResultIpcResource) {
      return ACPResult::IpcResource;
   } else if (result == ResultResource) {
      return ACPResult::Resource;
   }

   if (result == ResultNotInitialised) {
      return ACPResult::NotInitialised;
   }

   if (result == ResultAccountError) {
      return ACPResult::AccountError;
   }

   if (result == ResultUnsupported) {
      return ACPResult::Unsupported;
   }

   if (result == ResultSlcDataCorrupted) {
      ACPSendCOSFatalError(4, 0x1870AD, result, funcName, lineNo);
      return ACPResult::SlcDataCorrupted;
   } else if (result == ResultMlcDataCorrupted) {
      ACPSendCOSFatalError(2, 0x1870ae, result, funcName, lineNo);
      return ACPResult::MlcDataCorrupted;
   } else if (result == ResultUsbDataCorrupted) {
      ACPSendCOSFatalError(5, 0x187c67, result, funcName, lineNo);
      return ACPResult::UsbDataCorrupted;
   } else if (result == ResultDataCorrupted) {
      ACPSendCOSFatalError(3, 0x187c68, result, funcName, lineNo);
      return ACPResult::DataCorrupted;
   } else if (result == ResultDevice) {
      return ACPResult::Device;
   }

   if (result == ResultOddMediaNotReady) {
      return ACPResult::OddMediaNotReady;
   } else if (result == ResultOddMediaBroken) {
      return ACPResult::OddMediaBroken;
   } else if (result == ResultUsbMediaNotReady) {
      ACPSendCOSFatalError(6, 0x187499, result, funcName, lineNo);
      return ACPResult::UsbMediaNotReady;
   } else if (result == ResultUsbMediaBroken) {
      ACPSendCOSFatalError(5, 0x187c6a, result, funcName, lineNo);
      return ACPResult::UsbMediaBroken;
   } else if (result == ResultMediaNotReady) {
      return ACPResult::MediaNotReady;
   } else if (result == ResultMediaBroken) {
      return ACPResult::MediaBroken;
   } else if (result == ResultMediaWriteProtected) {
      return ACPResult::MediaWriteProtected;
   } else if (result == ResultUsbWriteProtected) {
      return ACPResult::UsbWriteProtected;
   } else if (result == ResultUsbWriteProtected) {
      return ACPResult::UsbWriteProtected;
   } else if (result == ResultMedia) {
      return ACPResult::Media;
   }

   if (result == ResultEncryptionError) {
      return ACPResult::EncryptionError;
   } else if (result == ResultMii) {
      return ACPResult::Mii;
   }

   if (result == ResultFsaFatal) {
      ACPSendCOSFatalError(1, 0x18748d, result, funcName, lineNo);
   } else if (result == ResultFsaAddClientFatal) {
      ACPSendCOSFatalError(1, 0x18748e, result, funcName, lineNo);
   } else if (result == ResultMcpTitleFatal) {
      ACPSendCOSFatalError(1, 0x18748f, result, funcName, lineNo);
   } else if (result == ResultMcpPatchFatal) {
      ACPSendCOSFatalError(1, 0x187490, result, funcName, lineNo);
   } else if (result == ResultMcpFatal) {
      ACPSendCOSFatalError(1, 0x187491, result, funcName, lineNo);
   } else if (result == ResultSaveFatal) {
      ACPSendCOSFatalError(1, 0x187492, result, funcName, lineNo);
   } else if (result == ResultUcFatal) {
      ACPSendCOSFatalError(1, 0x187493, result, funcName, lineNo);
   } else if (result == nn::ipc::ResultCAPABILITY_FAILED) {
      ACPSendCOSFatalError(1, 0x187494, result, funcName, lineNo);
   } else if (result == ResultFatal) {
      ACPSendCOSFatalError(1, 0x18748c, result, funcName, lineNo);
   } else {
      ACPSendCOSFatalError(1, 0x18749f, result, funcName, lineNo);
   }

   return ACPResult::GenericError;
}

} // namespace cafe::nn_acp
