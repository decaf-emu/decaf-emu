#include "nn_dbg_result_string.h"
#include "nn/acp/nn_acp_result.h"

namespace nn::dbg
{

const char *
GetLevelString(nn::Result result)
{
   if (result.isLegacyRepresentation()) {
      switch (result.level()) {
      case nn::Result::LEGACY_LEVEL_INFO:
         return "LEVEL_INFO";
      case nn::Result::LEGACY_LEVEL_RESET:
         return "LEVEL_RESET";
      case nn::Result::LEGACY_LEVEL_REINIT:
         return "LEVEL_REINIT";
      case nn::Result::LEGACY_LEVEL_PERMANENT:
         return "LEVEL_PERMANENT";
      case nn::Result::LEGACY_LEVEL_TEMPORARY:
         return "LEVEL_TEMPORARY";
      default:
         return "<unknown>";
      }
   } else {
      switch (result.level()) {
      case nn::Result::LEVEL_SUCCESS:
         return "LEVEL_SUCCESS";
      case nn::Result::LEVEL_FATAL:
         return "LEVEL_FATAL";
      case nn::Result::LEVEL_USAGE:
         return "LEVEL_USAGE";
      case nn::Result::LEVEL_STATUS:
         return "LEVEL_STATUS";
      case nn::Result::LEVEL_END:
         return "LEVEL_END";
      default:
         return "<unknown>";
      }
   }
}

const char *
GetModuleString(nn::Result result)
{
   if (result.isLegacyRepresentation()) {
      switch (result.module()) {
      case nn::Result::LEGACY_MODULE_COMMON:
         return "MODULE_COMMON";
      case nn::Result::LEGACY_MODULE_NN_KERNEL:
         return "MODULE_NN_KERNEL";
      case nn::Result::LEGACY_MODULE_NN_UTIL:
         return "MODULE_NN_UTIL";
      case nn::Result::LEGACY_MODULE_NN_FILE_SERVER:
         return "MODULE_NN_FILE_SERVER";
      case nn::Result::LEGACY_MODULE_NN_LOADER_SERVER:
         return "MODULE_NN_LOADER_SERVER";
      case nn::Result::LEGACY_MODULE_NN_TCB:
         return "MODULE_NN_TCB";
      case nn::Result::LEGACY_MODULE_NN_OS:
         return "MODULE_NN_OS";
      case nn::Result::LEGACY_MODULE_NN_DBG:
         return "MODULE_NN_DBG";
      case nn::Result::LEGACY_MODULE_NN_DMNT:
         return "MODULE_NN_DMNT";
      case nn::Result::LEGACY_MODULE_NN_PDN:
         return "MODULE_NN_PDN";
      case nn::Result::LEGACY_MODULE_NN_GX:
         return "MODULE_NN_GX";
      case nn::Result::LEGACY_MODULE_NN_I2C:
         return "MODULE_NN_I2C";
      case nn::Result::LEGACY_MODULE_NN_GPIO:
         return "MODULE_NN_GPIO";
      case nn::Result::LEGACY_MODULE_NN_DD:
         return "MODULE_NN_DD";
      case nn::Result::LEGACY_MODULE_NN_CODEC:
         return "MODULE_NN_CODEC";
      case nn::Result::LEGACY_MODULE_NN_SPI:
         return "MODULE_NN_SPI";
      case nn::Result::LEGACY_MODULE_NN_PXI:
         return "MODULE_NN_PXI";
      case nn::Result::LEGACY_MODULE_NN_FS:
         return "MODULE_NN_FS";
      case nn::Result::LEGACY_MODULE_NN_DI:
         return "MODULE_NN_DI";
      case nn::Result::LEGACY_MODULE_NN_HID:
         return "MODULE_NN_HID";
      case nn::Result::LEGACY_MODULE_NN_CAMERA:
         return "MODULE_NN_CAMERA";
      case nn::Result::LEGACY_MODULE_NN_PI:
         return "MODULE_NN_PI";
      case nn::Result::LEGACY_MODULE_NN_PM:
         return "MODULE_NN_PM";
      case nn::Result::LEGACY_MODULE_NN_PMLOW:
         return "MODULE_NN_PMLOW";
      case nn::Result::LEGACY_MODULE_NN_FSI:
         return "MODULE_NN_FSI";
      case nn::Result::LEGACY_MODULE_NN_SRV:
         return "MODULE_NN_SRV";
      case nn::Result::LEGACY_MODULE_NN_NDM:
         return "MODULE_NN_NDM";
      case nn::Result::LEGACY_MODULE_NN_NWM:
         return "MODULE_NN_NWM";
      case nn::Result::LEGACY_MODULE_NN_SOCKET:
         return "MODULE_NN_SOCKET";
      case nn::Result::LEGACY_MODULE_NN_LDR:
         return "MODULE_NN_LDR";
      case nn::Result::LEGACY_MODULE_NN_ACC:
         return "MODULE_NN_ACC";
      case nn::Result::LEGACY_MODULE_NN_ROMFS:
         return "MODULE_NN_ROMFS";
      case nn::Result::LEGACY_MODULE_NN_AM:
         return "MODULE_NN_AM";
      case nn::Result::LEGACY_MODULE_NN_HIO:
         return "MODULE_NN_HIO";
      case nn::Result::LEGACY_MODULE_NN_UPDATER:
         return "MODULE_NN_UPDATER";
      case nn::Result::LEGACY_MODULE_NN_MIC:
         return "MODULE_NN_MIC";
      case nn::Result::LEGACY_MODULE_NN_FND:
         return "MODULE_NN_FND";
      case nn::Result::LEGACY_MODULE_NN_MP:
         return "MODULE_NN_MP";
      case nn::Result::LEGACY_MODULE_NN_MPWL:
         return "MODULE_NN_MPWL";
      case nn::Result::LEGACY_MODULE_NN_AC:
         return "MODULE_NN_AC";
      case nn::Result::LEGACY_MODULE_NN_HTTP:
         return "MODULE_NN_HTTP";
      case nn::Result::LEGACY_MODULE_NN_DSP:
         return "MODULE_NN_DSP";
      case nn::Result::LEGACY_MODULE_NN_SND:
         return "MODULE_NN_SND";
      case nn::Result::LEGACY_MODULE_NN_DLP:
         return "MODULE_NN_DLP";
      case nn::Result::LEGACY_MODULE_NN_HIOLOW:
         return "MODULE_NN_HIOLOW";
      case nn::Result::LEGACY_MODULE_NN_CSND:
         return "MODULE_NN_CSND";
      case nn::Result::LEGACY_MODULE_NN_SSL:
         return "MODULE_NN_SSL";
      case nn::Result::LEGACY_MODULE_NN_AMLOW:
         return "MODULE_NN_AMLOW";
      case nn::Result::LEGACY_MODULE_NN_NEX:
         return "MODULE_NN_NEX";
      case nn::Result::LEGACY_MODULE_NN_FRIENDS:
         return "MODULE_NN_FRIENDS";
      case nn::Result::LEGACY_MODULE_NN_RDT:
         return "MODULE_NN_RDT";
      case nn::Result::LEGACY_MODULE_NN_APPLET:
         return "MODULE_NN_APPLET";
      case nn::Result::LEGACY_MODULE_NN_NIM:
         return "MODULE_NN_NIM";
      case nn::Result::LEGACY_MODULE_NN_PTM:
         return "MODULE_NN_PTM";
      case nn::Result::LEGACY_MODULE_NN_MIDI:
         return "MODULE_NN_MIDI";
      case nn::Result::LEGACY_MODULE_NN_MC:
         return "MODULE_NN_MC";
      case nn::Result::LEGACY_MODULE_NN_SWC:
         return "MODULE_NN_SWC";
      case nn::Result::LEGACY_MODULE_NN_FATFS:
         return "MODULE_NN_FATFS";
      case nn::Result::LEGACY_MODULE_NN_NGC:
         return "MODULE_NN_NGC";
      case nn::Result::LEGACY_MODULE_NN_CARD:
         return "MODULE_NN_CARD";
      case nn::Result::LEGACY_MODULE_NN_CARDNOR:
         return "MODULE_NN_CARDNOR";
      case nn::Result::LEGACY_MODULE_NN_SDMC:
         return "MODULE_NN_SDMC";
      case nn::Result::LEGACY_MODULE_NN_BOSS:
         return "MODULE_NN_BOSS";
      case nn::Result::LEGACY_MODULE_NN_DBM:
         return "MODULE_NN_DBM";
      case nn::Result::LEGACY_MODULE_NN_CFG:
         return "MODULE_NN_CFG";
      case nn::Result::LEGACY_MODULE_NN_PS:
         return "MODULE_NN_PS";
      case nn::Result::LEGACY_MODULE_NN_CEC:
         return "MODULE_NN_CEC";
      case nn::Result::LEGACY_MODULE_NN_IR:
         return "MODULE_NN_IR";
      case nn::Result::LEGACY_MODULE_NN_UDS:
         return "MODULE_NN_UDS";
      case nn::Result::LEGACY_MODULE_NN_PL:
         return "MODULE_NN_PL";
      case nn::Result::LEGACY_MODULE_NN_CUP:
         return "MODULE_NN_CUP";
      case nn::Result::LEGACY_MODULE_NN_GYROSCOPE:
         return "MODULE_NN_GYROSCOPE";
      case nn::Result::LEGACY_MODULE_NN_MCU:
         return "MODULE_NN_MCU";
      case nn::Result::LEGACY_MODULE_NN_NS:
         return "MODULE_NN_NS";
      case nn::Result::LEGACY_MODULE_NN_NEWS:
         return "MODULE_NN_NEWS";
      case nn::Result::LEGACY_MODULE_NN_RO:
         return "MODULE_NN_RO";
      case nn::Result::LEGACY_MODULE_NN_GD:
         return "MODULE_NN_GD";
      case nn::Result::LEGACY_MODULE_NN_CARDSPI:
         return "MODULE_NN_CARDSPI";
      case nn::Result::LEGACY_MODULE_NN_EC:
         return "MODULE_NN_EC";
      case nn::Result::LEGACY_MODULE_NN_WEBBRS:
         return "MODULE_NN_WEBBRS";
      case nn::Result::LEGACY_MODULE_NN_TEST:
         return "MODULE_NN_TEST";
      case nn::Result::LEGACY_MODULE_NN_ENC:
         return "MODULE_NN_ENC";
      case nn::Result::LEGACY_MODULE_NN_PIA:
         return "MODULE_NN_PIA";
      case nn::Result::LEGACY_MODULE_APPLICATION:
         return "MODULE_APPLICATION";
      }
   } else {
      switch (result.module()) {
      case nn::Result::MODULE_COMMON:
         return "MODULE_COMMON";
      case nn::Result::MODULE_NN_IPC:
         return "MODULE_NN_IPC";
      case nn::Result::MODULE_NN_BOSS:
         return "MODULE_NN_BOSS";
      case nn::Result::MODULE_NN_ACP:
         return "MODULE_NN_ACP";
      case nn::Result::MODULE_NN_IOS:
         return "MODULE_NN_IOS";
      case nn::Result::MODULE_NN_NIM:
         return "MODULE_NN_NIM";
      case nn::Result::MODULE_NN_PDM:
         return "MODULE_NN_PDM";
      case nn::Result::MODULE_NN_ACT:
         return "MODULE_NN_ACT";
      case nn::Result::MODULE_NN_NGC:
         return "MODULE_NN_NGC";
      case nn::Result::MODULE_NN_ECA:
         return "MODULE_NN_ECA";
      case nn::Result::MODULE_NN_NUP:
         return "MODULE_NN_NUP";
      case nn::Result::MODULE_NN_NDM:
         return "MODULE_NN_NDM";
      case nn::Result::MODULE_NN_FP:
         return "MODULE_NN_FP";
      case nn::Result::MODULE_NN_AC:
         return "MODULE_NN_AC";
      case nn::Result::MODULE_NN_CONNTEST:
         return "MODULE_NN_CONNTEST";
      case nn::Result::MODULE_NN_DRMAPP:
         return "MODULE_NN_DRMAPP";
      case nn::Result::MODULE_NN_TELNET:
         return "MODULE_NN_TELNET";
      case nn::Result::MODULE_NN_OLV:
         return "MODULE_NN_OLV";
      case nn::Result::MODULE_NN_VCTL:
         return "MODULE_NN_VCTL";
      case nn::Result::MODULE_NN_NEIA:
         return "MODULE_NN_NEIA";
      case nn::Result::MODULE_NN_SPM:
         return "MODULE_NN_SPM";
      case nn::Result::MODULE_NN_EMD:
         return "MODULE_NN_EMD";
      case nn::Result::MODULE_NN_EC:
         return "MODULE_NN_EC";
      case nn::Result::MODULE_NN_CIA:
         return "MODULE_NN_CIA";
      case nn::Result::MODULE_NN_SL:
         return "MODULE_NN_SL";
      case nn::Result::MODULE_NN_ECO:
         return "MODULE_NN_ECO";
      case nn::Result::MODULE_NN_TRIAL:
         return "MODULE_NN_TRIAL";
      case nn::Result::MODULE_NN_NFP:
         return "MODULE_NN_NFP";
      case nn::Result::MODULE_NN_TEST:
         return "MODULE_NN_TEST";
      }
   }

   return "<unknown>";
}

const char *
GetSummaryString(nn::Result result)
{
   if (!result.isLegacyRepresentation()) {
      return "SUMMARY_SUCCESS";
   }

   switch (result.legacySummary()) {
   case nn::Result::LEGACY_SUMMARY_SUCCESS:
      return "SUMMARY_SUCCESS";
   case nn::Result::LEGACY_SUMMARY_NOTHING_HAPPENED:
      return "SUMMARY_NOTHING_HAPPENED";
   case nn::Result::LEGACY_SUMMARY_WOULD_BLOCK:
      return "SUMMARY_WOULD_BLOCK";
   case nn::Result::LEGACY_SUMMARY_OUT_OF_RESOURCE:
      return "SUMMARY_OUT_OF_RESOURCE";
   case nn::Result::LEGACY_SUMMARY_NOT_FOUND:
      return "SUMMARY_NOT_FOUND";
   case nn::Result::LEGACY_SUMMARY_INVALID_STATE:
      return "SUMMARY_INVALID_STATE";
   case nn::Result::LEGACY_SUMMARY_NOT_SUPPORTED:
      return "SUMMARY_NOT_SUPPORTED";
   case nn::Result::LEGACY_SUMMARY_INVALID_ARGUMENT:
      return "SUMMARY_INVALID_ARGUMENT";
   case nn::Result::LEGACY_SUMMARY_WRONG_ARGUMENT:
      return "SUMMARY_WRONG_ARGUMENT";
   case nn::Result::LEGACY_SUMMARY_CANCELLED:
      return "SUMMARY_CANCELLED";
   case nn::Result::LEGACY_SUMMARY_STATUS_CHANGED:
      return "SUMMARY_STATUS_CHANGED";
   case nn::Result::LEGACY_SUMMARY_INTERNAL:
      return "SUMMARY_INTERNAL";
   }

   return "<unknown>";
}

static const char *
GetLegacyDescriptionString(nn::Result result)
{
   switch (result.description()) {
   case nn::Result::LEGACY_DESCRIPTION_SUCCESS:
      return "DESCRIPTION_SUCCESS";
   case nn::Result::LEGACY_DESCRIPTION_TIMEOUT:
      return "DESCRIPTION_TIMEOUT";
   case nn::Result::LEGACY_DESCRIPTION_OUT_OF_RANGE:
      return "DESCRIPTION_OUT_OF_RANGE";
   case nn::Result::LEGACY_DESCRIPTION_ALREADY_EXISTS:
      return "DESCRIPTION_ALREADY_EXISTS";
   case nn::Result::LEGACY_DESCRIPTION_CANCEL_REQUESTED:
      return "DESCRIPTION_CANCEL_REQUESTED";
   case nn::Result::LEGACY_DESCRIPTION_NOT_FOUND:
      return "DESCRIPTION_NOT_FOUND";
   case nn::Result::LEGACY_DESCRIPTION_ALREADY_INITIALIZED:
      return "DESCRIPTION_ALREADY_INITIALIZED";
   case nn::Result::LEGACY_DESCRIPTION_NOT_INITIALIZED:
      return "DESCRIPTION_NOT_INITIALIZED";
   case nn::Result::LEGACY_DESCRIPTION_INVALID_HANDLE:
      return "DESCRIPTION_INVALID_HANDLE";
   case nn::Result::LEGACY_DESCRIPTION_INVALID_POINTER:
      return "DESCRIPTION_INVALID_POINTER";
   case nn::Result::LEGACY_DESCRIPTION_INVALID_ADDRESS:
      return "DESCRIPTION_INVALID_ADDRESS";
   case nn::Result::LEGACY_DESCRIPTION_NOT_IMPLEMENTED:
      return "DESCRIPTION_NOT_IMPLEMENTED";
   case nn::Result::LEGACY_DESCRIPTION_OUT_OF_MEMORY:
      return "DESCRIPTION_OUT_OF_MEMORY";
   case nn::Result::LEGACY_DESCRIPTION_MISALIGNED_SIZE:
      return "DESCRIPTION_MISALIGNED_SIZE";
   case nn::Result::LEGACY_DESCRIPTION_MISALIGNED_ADDRESS:
      return "DESCRIPTION_MISALIGNED_ADDRESS";
   case nn::Result::LEGACY_DESCRIPTION_BUSY:
      return "DESCRIPTION_BUSY";
   case nn::Result::LEGACY_DESCRIPTION_NO_DATA:
      return "DESCRIPTION_NO_DATA";
   case nn::Result::LEGACY_DESCRIPTION_INVALID_COMBINATION:
      return "DESCRIPTION_INVALID_COMBINATION";
   case nn::Result::LEGACY_DESCRIPTION_INVALID_ENUM_VALUE:
      return "DESCRIPTION_INVALID_ENUM_VALUE";
   case nn::Result::LEGACY_DESCRIPTION_INVALID_SIZE:
      return "DESCRIPTION_INVALID_SIZE";
   case nn::Result::LEGACY_DESCRIPTION_ALREADY_DONE:
      return "DESCRIPTION_ALREADY_DONE";
   case nn::Result::LEGACY_DESCRIPTION_NOT_AUTHORIZED:
      return "DESCRIPTION_NOT_AUTHORIZED";
   case nn::Result::LEGACY_DESCRIPTION_TOO_LARGE:
      return "DESCRIPTION_TOO_LARGE";
   case nn::Result::LEGACY_DESCRIPTION_INVALID_SELECTION:
      return "DESCRIPTION_INVALID_SELECTION";
   default:
      return "<unknown>";
   }
}

static const char *
GetAcpDescriptionString(nn::Result result)
{
   if (result == acp::ResultInvalidParameter) {
      return "INVALID_PARAMETER";
   } else if (result == acp::ResultInvalidFile) {
      return "INVALID_FILE";
   } else if (result == acp::ResultInvalidXmlFile) {
      return "INVALID_XML_FILE";
   } else if (result == acp::ResultFileAccessMode) {
      return "FILE_ACCESS_MODE";
   } else if (result == acp::ResultInvalidNetworkTime) {
      return "INVALID_NETWORK_TIME";
   } else if (result == acp::ResultInvalid) {
      return "INVALID";
   }

   if (result == acp::ResultFileNotFound) {
      return "FILE_NOT_FOUND";
   } else if (result == acp::ResultDirNotFound) {
      return "DIR_NOT_FOUND";
   } else if (result == acp::ResultDeviceNotFound) {
      return "DEVICE_NOT_FOUND";
   } else if (result == acp::ResultTitleNotFound) {
      return "TITLE_NOT_FOUND";
   } else if (result == acp::ResultApplicationNotFound) {
      return "APPLICATION_NOT_FOUND";
   } else if (result == acp::ResultSystemConfigNotFound) {
      return "SYSTEM_CONFIG_NOT_FOUND";
   } else if (result == acp::ResultXmlItemNotFound) {
      return "XML_ITEM_NOT_FOUND";
   } else if (result == acp::ResultNotFound) {
      return "NOT_FOUND";
   }

   if (result == acp::ResultFileAlreadyExists) {
      return "FILE_ALREADY_EXISTS";
   } else if (result == acp::ResultDirAlreadyExists) {
      return "DIR_ALREADY_EXISTS";
   } else if (result == acp::ResultAlreadyExists) {
      return "ALREADY_EXISTS";
   }

   if (result == acp::ResultAlreadyDone) {
      return "ALREADY_DONE";
   }

   if (result == acp::ResultInvalidRegion) {
      return "INVALID_REGION";
   } else if (result == acp::ResultRestrictedRating) {
      return "RESTRICTED_RATING";
   } else if (result == acp::ResultNotPresentRating) {
      return "NOT_PRESENT_RATING";
   } else if (result == acp::ResultPendingRating) {
      return "PENDING_RATING";
   } else if (result == acp::ResultNetSettingRequired) {
      return "NET_SETTING_REQUIRED";
   } else if (result == acp::ResultNetAccountRequired) {
      return "NET_ACCOUNT_REQUIRED";
   } else if (result == acp::ResultNetAccountError) {
      return "NET_ACCOUNT_ERROR";
   } else if (result == acp::ResultBrowserRequired) {
      return "BROWSER_REQUIRED";
   } else if (result == acp::ResultOlvRequired) {
      return "OLV_REQUIRED";
   } else if (result == acp::ResultPincodeRequired) {
      return "PINCODE_REQUIRED";
   } else if (result == acp::ResultIncorrectPincode) {
      return "INCORRECT_PINCODE";
   } else if (result == acp::ResultInvalidLogo) {
      return "INVALID_LOGO";
   } else if (result == acp::ResultDemoExpiredNumber) {
      return "DEMO_EXPIRED_NUMBER";
   } else if (result == acp::ResultDrcRequired) {
      return "DRC_REQUIRED";
   } else if (result == acp::ResultAuthentication) {
      return "AUTHENTICATION";
   }

   if (result == acp::ResultNoFilePermission) {
      return "NO_FILE_PERMISSION";
   } else if (result == acp::ResultNoDirPermission) {
      return "NO_DIR_PERMISSION";
   } else if (result == acp::ResultNoPermission) {
      return "NO_PERMISSION";
   }

   if (result == acp::ResultUsbStorageNotReady) {
      return "USB_STORAGE_NOT_READY";
   } else if (result == acp::ResultBusy) {
      return "BUSY";
   }

   if (result == acp::ResultCancelled) {
      return "CANCELLED";
   }

   if (result == acp::ResultDeviceFull) {
      return "DEVICE_FULL";
   } else if (result == acp::ResultJournalFull) {
      return "JOURNAL_FULL";
   } else if (result == acp::ResultSystemMemory) {
      return "SYSTEM_MEMORY";
   } else if (result == acp::ResultFsResource) {
      return "FS_RESOURCE";
   } else if (result == acp::ResultIpcResource) {
      return "IPC_RESOURCE";
   } else if (result == acp::ResultResource) {
      return "RESOURCE";
   }

   if (result == acp::ResultNotInitialised) {
      return "NOT_INITIALISED";
   }

   if (result == acp::ResultAccountError) {
      return "ACCOUNT_ERROR";
   }

   if (result == acp::ResultUnsupported) {
      return "UNSUPPORTED";
   }

   if (result == acp::ResultDataCorrupted) {
      return "DATA_CORRUPTED";
   } else if (result == acp::ResultSlcDataCorrupted) {
      return "SLC_DATA_CORRUPTED";
   } else if (result == acp::ResultMlcDataCorrupted) {
      return "MLC_DATA_CORRUPTED";
   } else if (result == acp::ResultUsbDataCorrupted) {
      return "USB_DATA_CORRUPTED";
   } else if (result == acp::ResultDevice) {
      return "DEVICE";
   }

   if (result == acp::ResultMediaNotReady) {
      return "MEDIA_NOT_READY";
   } else if (result == acp::ResultMediaBroken) {
      return "MEDIA_BROKEN";
   } else if (result == acp::ResultOddMediaNotReady) {
      return "ODD_MEDIA_NOT_READY";
   } else if (result == acp::ResultOddMediaBroken) {
      return "ODD_MEDIA_BROKEN";
   } else if (result == acp::ResultUsbMediaNotReady) {
      return "USB_MEDIA_NOT_READY";
   } else if (result == acp::ResultUsbMediaBroken) {
      return "USB_MEDIA_BROKEN";
   } else if (result == acp::ResultMediaWriteProtected) {
      return "MEDIA_WRITE_PROTECTED";
   } else if (result == acp::ResultUsbWriteProtected) {
      return "USB_WRITE_PROTECTED";
   } else if (result == acp::ResultMedia) {
      return "MEDIA";
   }

   if (result == acp::ResultEncryptionError) {
      return "ENCRYPTION_ERROR";
   } else if (result == acp::ResultMii) {
      return "MII";
   }

   if (result == acp::ResultFsaFatal) {
      return "FSA_FATAL";
   } else if (result == acp::ResultFsaAddClientFatal) {
      return "FSA_ADD_CLIENT_FATAL";
   } else if (result == acp::ResultMcpTitleFatal) {
      return "MCP_TITLE_FATAL";
   } else if (result == acp::ResultMcpPatchFatal) {
      return "MCP_PATCH_FATAL";
   } else if (result == acp::ResultMcpFatal) {
      return "MCP_FATAL";
   } else if (result == acp::ResultSaveFatal) {
      return "SAVE_FATAL";
   } else if (result == acp::ResultUcFatal) {
      return "UC_FATAL";
   } else if (result == acp::ResultFatal) {
      return "FATAL";
   }

   return "<unknown>";
}

const char *
GetDescriptionString(nn::Result result)
{
   if (result.isLegacyRepresentation()) {
      return GetLegacyDescriptionString(result);
   }

   switch (result.module()) {
   case nn::Result::MODULE_NN_ACP:
      return GetAcpDescriptionString(result);
   }

   return "<unknown>";
}

} // namespace nn::dbg
