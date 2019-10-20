#pragma once
#include "nn/nn_result.h"

namespace nn::acp
{

static constexpr Result ResultSuccess {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_SUCCESS, 128
};


static constexpr const auto ResultInvalid =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_USAGE, 0x6400, 0x9600>();

static constexpr const auto ResultInvalidParameter =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_USAGE, 0x6480, 0x6500>();

static constexpr const auto ResultInvalidFile =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_USAGE, 0x6500, 0x6580>();

static constexpr const auto ResultInvalidXmlFile =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_USAGE, 0x6580, 0x6600>();

static constexpr const auto ResultFileAccessMode =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_USAGE, 0x6600, 0x6680>();

static constexpr const auto ResultInvalidNetworkTime =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_USAGE, 0x6680, 0x6700>();


static constexpr const auto ResultNotFound =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0xFA00, 0x12C00>();

static constexpr const auto ResultFileNotFound =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0xFA80, 0xFB00>();

static constexpr const auto ResultDirNotFound =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0xFB00, 0xFB80>();

static constexpr const auto ResultDeviceNotFound =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0xFB80, 0xFC00>();

static constexpr const auto ResultTitleNotFound =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0xFC00, 0xFC80>();

static constexpr const auto ResultApplicationNotFound =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0xFC80, 0xFD00>();

static constexpr const auto ResultSystemConfigNotFound =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0xFD00, 0xFD80>();

static constexpr const auto ResultXmlItemNotFound =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0xFD80, 0xFE00>();


static constexpr const auto ResultAlreadyExists =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x12C00, 0x15E00>();

static constexpr const auto ResultFileAlreadyExists =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x12C80, 0x12D00>();

static constexpr const auto ResultDirAlreadyExists =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x12D00, 0x12D80>();


static constexpr const auto ResultAlreadyDone =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x15E00, 0x19000>();


static constexpr const auto ResultAuthentication =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F400, 0x22600>();

static constexpr const auto ResultInvalidRegion =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F480, 0x1F500>();

static constexpr const auto ResultRestrictedRating =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F500, 0x1F580>();

static constexpr const auto ResultNotPresentRating =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F580, 0x1F600>();

static constexpr const auto ResultPendingRating =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F600, 0x1F680>();

static constexpr const auto ResultNetSettingRequired =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F680, 0x1F700>();

static constexpr const auto ResultNetAccountRequired =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F700, 0x1F780>();

static constexpr const auto ResultNetAccountError =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F780, 0x1F800>();

static constexpr const auto ResultBrowserRequired =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F800, 0x1F880>();

static constexpr const auto ResultOlvRequired =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F880, 0x1F900>();

static constexpr const auto ResultPincodeRequired =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F900, 0x1F980>();

static constexpr const auto ResultIncorrectPincode =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1F980, 0x1FA00>();

static constexpr const auto ResultInvalidLogo =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1FA00, 0x1FA80>();

static constexpr const auto ResultDemoExpiredNumber =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1FA80, 0x1FB00>();

static constexpr const auto ResultDrcRequired =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x1FB00, 0x1FB80>();


static constexpr const auto ResultNoPermission =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x22600, 0x25800>();

static constexpr const auto ResultNoFilePermission =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x22680, 0x22700>();

static constexpr const auto ResultNoDirPermission =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x22700, 0x22780>();


static constexpr const auto ResultBusy =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x28A00, 0x2BC00>();

static constexpr const auto ResultUsbStorageNotReady =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x28A80, 0x28B00>();


static constexpr const auto ResultCancelled =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x2BC00, 0x2EE00>();


static constexpr const auto ResultResource =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x2EE00, 0x32000>();

static constexpr const auto ResultDeviceFull =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x2EE80, 0x2EF00>();

static constexpr const auto ResultJournalFull =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x2EF00, 0x2EF80>();

static constexpr const auto ResultSystemMemory =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x2EF80, 0x2F000>();

static constexpr const auto ResultFsResource =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x2F000, 0x2F080>();

static constexpr const auto ResultIpcResource =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x2F080, 0x2F100>();


static constexpr const auto ResultNotInitialised =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x32000, 0x35200>();


static constexpr const auto ResultAccountError =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x35200, 0x38400>();


static constexpr const auto ResultUnsupported =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x38400, 0x3B600>();


static constexpr const auto ResultDevice =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x3E800, 0x41A00>();

static constexpr const auto ResultDataCorrupted =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x3E880, 0x3E900>();

static constexpr const auto ResultSlcDataCorrupted =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x3E900, 0x3E980>();

static constexpr const auto ResultMlcDataCorrupted =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x3E980, 0x3EA00>();

static constexpr const auto ResultUsbDataCorrupted =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x3EA00, 0x3EA80>();


static constexpr const auto ResultMedia =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x41A00, 0x44C00>();

static constexpr const auto ResultMediaNotReady =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x41A80, 0x41B00>();

static constexpr const auto ResultMediaBroken =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x41B00, 0x41B80>();

static constexpr const auto ResultOddMediaNotReady =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x41B80, 0x41C00>();

static constexpr const auto ResultOddMediaBroken =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x41C00, 0x41C80>();

static constexpr const auto ResultUsbMediaNotReady =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x41C80, 0x41D00>();

static constexpr const auto ResultUsbMediaBroken =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x41D00, 0x41D80>();

static constexpr const auto ResultMediaWriteProtected =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x41D80, 0x41E00>();

static constexpr const auto ResultUsbWriteProtected =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x41E00, 0x41E80>();


static constexpr const auto ResultMii =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x44C00, 0x47E00>();

static constexpr const auto ResultEncryptionError =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_STATUS, 0x44C80, 0x44D00>();


static constexpr const auto ResultFatal =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_FATAL, 0x7D000, 0x80000>();

static constexpr const auto ResultFsaFatal =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_FATAL, 0x7D080, 0x7D100>();

static constexpr const auto ResultFsaAddClientFatal =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_FATAL, 0x7D100, 0x7D180>();

static constexpr const auto ResultMcpTitleFatal =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_FATAL, 0x7D180, 0x7D200>();

static constexpr const auto ResultMcpPatchFatal =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_FATAL, 0x7D200, 0x7D280>();

static constexpr const auto ResultMcpFatal =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_FATAL, 0x7D280, 0x7D300>();

static constexpr const auto ResultSaveFatal =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_FATAL, 0x7D300, 0x7D380>();

static constexpr const auto ResultUcFatal =
   ResultRange<Result::MODULE_NN_ACP, Result::LEVEL_FATAL, 0x7D380, 0x7D400>();


} // namespace nn::acp
