#pragma once
#include "nn/nn_result.h"
#include "ios/ios_enum.h"

namespace nn::ios
{

static constexpr Result ResultOK {
   Result::MODULE_NN_IOS, Result::LEVEL_SUCCESS, 0
};

static constexpr Result ResultAccess {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x100
};

static constexpr Result ResultExists {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x180
};

static constexpr Result ResultIntr {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x200
};

static constexpr Result ResultInvalid {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x280
};

static constexpr Result ResultMax {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x300
};

static constexpr Result ResultNoExists {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x380
};

static constexpr Result ResultQEmpty {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x400
};

static constexpr Result ResultQFull {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x480
};

static constexpr Result ResultUnknown {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x500
};

static constexpr Result ResultNotReady {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x580
};

static constexpr Result ResultInvalidObjType {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x600
};

static constexpr Result ResultInvalidVersion {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x680
};

static constexpr Result ResultInvalidSigner {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x700
};

static constexpr Result ResultFailCheckValue {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x780
};

static constexpr Result ResultFailInternal {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x800
};

static constexpr Result ResultFailAlloc {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x880
};

static constexpr Result ResultInvalidSize {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x900
};

static constexpr Result ResultNoLink {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x980
};

static constexpr Result ResultANFailed {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xA00
};

static constexpr Result ResultMaxSemCount {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xA80
};

static constexpr Result ResultSemUnavailable {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xB00
};

static constexpr Result ResultInvalidHandle {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xB80
};

static constexpr Result ResultInvalidArg {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xC00
};

static constexpr Result ResultNoResource {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xC80
};

static constexpr Result ResultBusy {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xD00
};

static constexpr Result ResultTimeout {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xD80
};

static constexpr Result ResultAlignment {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xE00
};

static constexpr Result ResultBSP {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xE80
};

static constexpr Result ResultDataPending {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xF00
};

static constexpr Result ResultExpired {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0xF80
};

static constexpr Result ResultNoReadAccess {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x1000
};

static constexpr Result ResultNoWriteAccess {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x1080
};

static constexpr Result ResultNoReadWriteAccess {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x1100
};

static constexpr Result ResultClientTxnLimit {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x1180
};

static constexpr Result ResultStaleHandle {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x1200
};

static constexpr Result ResultUnknownValue {
   Result::MODULE_NN_IOS, Result::LEVEL_STATUS, 0x3180
};

Result
convertError(::ios::Error error);

} // namespace nn::ios
