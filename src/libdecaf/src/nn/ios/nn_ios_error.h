#pragma once
#include "nn/nn_result.h"
#include "ios/ios_enum.h"

namespace nn::ios
{

static constexpr Result ResultOK {
   Result::MODULE_NN_IOS, Result::LEVEL_SUCCESS, 0
};

static constexpr const auto ResultAccess =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x100, 0x180>();

static constexpr const auto ResultExists =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x180, 0x200>();

static constexpr const auto ResultIntr =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x200, 0x280>();

static constexpr const auto ResultInvalid =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x280, 0x300>();

static constexpr const auto ResultMax =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x300, 0x380>();

static constexpr const auto ResultNoExists =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x380, 0x400>();

static constexpr const auto ResultQEmpty =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x400, 0x480>();

static constexpr const auto ResultQFull =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x480, 0x500>();

static constexpr const auto ResultUnknown =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x500, 0x580>();

static constexpr const auto ResultNotReady =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x580, 0x600>();

static constexpr const auto ResultInvalidObjType =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x600, 0x680>();

static constexpr const auto ResultInvalidVersion =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x680, 0x700>();

static constexpr const auto ResultInvalidSigner =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x700, 0x780>();

static constexpr const auto ResultFailCheckValue =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x780, 0x800>();

static constexpr const auto ResultFailInternal =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x800, 0x880>();

static constexpr const auto ResultFailAlloc =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x880, 0x900>();

static constexpr const auto ResultInvalidSize =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x900, 0x980>();

static constexpr const auto ResultNoLink =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x980, 0xA00>();

static constexpr const auto ResultANFailed =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xA00, 0xA80>();

static constexpr const auto ResultMaxSemCount =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xA80, 0xB00>();

static constexpr const auto ResultSemUnavailable =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xB00, 0xB80>();

static constexpr const auto ResultInvalidHandle =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xB80, 0xC00>();

static constexpr const auto ResultInvalidArg =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xC00, 0xC80>();

static constexpr const auto ResultNoResource =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xC80, 0xD00>();

static constexpr const auto ResultBusy =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xD00, 0xD80>();

static constexpr const auto ResultTimeout =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xD80, 0xE00>();

static constexpr const auto ResultAlignment =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xE00, 0xE80>();

static constexpr const auto ResultBSP =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xE80, 0xF00>();

static constexpr const auto ResultDataPending =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xF00, 0xF80>();

static constexpr const auto ResultExpired =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xF80, 0x1000>();

static constexpr const auto ResultNoReadAccess =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x1000, 0x1080>();

static constexpr const auto ResultNoWriteAccess =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x1080, 0x1100>();

static constexpr const auto ResultNoReadWriteAccess =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x1100, 0x1180>();

static constexpr const auto ResultClientTxnLimit =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x1180, 0x1200>();

static constexpr const auto ResultStaleHandle =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x1200, 0x1280>();

static constexpr const auto ResultUnknownValue =
   ResultRange<Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x3180, 0x3180>();

Result
convertError(::ios::Error error);

} // namespace nn::ios
