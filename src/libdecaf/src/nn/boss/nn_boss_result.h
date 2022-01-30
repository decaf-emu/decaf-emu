#pragma once
#include "nn/nn_result.h"

namespace nn::boss
{

static constexpr Result ResultSuccess {
   Result::MODULE_NN_BOSS, Result::LEVEL_SUCCESS, 0x00080
};

static constexpr Result ResultNotInitialised {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03200
};

static constexpr Result ResultLibraryNotInitialiased {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03280
};

static constexpr Result ResultInvalid {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03700
};

static constexpr Result ResultInvalidParameter {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03780
};

static constexpr Result ResultInvalidFormat {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03800
};

static constexpr Result ResultInvalidAccount {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03880
};

static constexpr Result ResultInvalidTitle {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x03900
};

static constexpr Result ResultNoSupport {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 0x04100
};

static constexpr Result ResultInitialized {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1f400
};

static constexpr Result ResultNotExist {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1f900
};

static constexpr Result ResultFileNotExist {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1f980
};

static constexpr Result ResultBossStorageNotExist {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1fa00
};

static constexpr Result ResultDbNotExist {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1fa80
};

static constexpr Result ResultRecordNotExist {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x1fb00
};

static constexpr Result ResultNotCompleted {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x20300
};

static constexpr Result ResultNotPermitted {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x20800
};

static constexpr Result ResultFull {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x20d00
};

static constexpr Result ResultSizeFull {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x20d80
};

static constexpr Result ResultCountFull {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x20e00
};

static constexpr Result ResultFinished {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x21200
};

static constexpr Result ResultServiceFinished {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x21280
};

static constexpr Result ResultCanceled {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x21700
};

static constexpr Result ResultStoppedByPolicylist {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x21c00
};

static constexpr Result ResultAlreadyExist {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x22100
};

static constexpr Result ResultCannotGetNetworkTime {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x22600
};

static constexpr Result ResultNotNetworkAccount {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x22b00
};

static constexpr Result ResultRestrictedByParentalControl {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x23000
};

static constexpr Result ResultDisableUploadConsoleInformation {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x23500
};

static constexpr Result ResultNotConnectNetwork {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x23a00
};

static constexpr Result ResultRestrictedByParentalControlTotalEnable {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x23f00
};

static constexpr Result ResultNotFound {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x24400
};

static constexpr Result ResultBossStorageNotFound {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x24480
};

static constexpr Result ResultHTTPError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x3e800
};

static constexpr Result ResultFsError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4E200
};

static constexpr Result ResultFsErrorNotInit {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e280
};

static constexpr Result ResultFsErrorBusy {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e300
};

static constexpr Result ResultFsErrorCanceled {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e380
};

static constexpr Result ResultFsErrorEndOfDirectory {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e400
};

static constexpr Result ResultFsErrorEndOfFile {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e480
};

static constexpr Result ResultFsErrorMaxMountpoints {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e500
};

static constexpr Result ResultFsErrorMaxVolumes {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e580
};

static constexpr Result ResultFsErrorMaxClients {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e600
};

static constexpr Result ResultFsErrorMaxFiles {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e680
};

static constexpr Result ResultFsErrorMaxDirs {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e700
};

static constexpr Result ResultFsErrorAlreadyOpen {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e780
};

static constexpr Result ResultFsErrorAlreadyExists {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e800
};

static constexpr Result ResultFsErrorNotFound {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e880
};

static constexpr Result ResultFsErrorNotEmpty {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e900
};

static constexpr Result ResultFsErrorAccessError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4e980
};

static constexpr Result ResultFsErrorPermissionError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ea00
};

static constexpr Result ResultFsErrorDataCorrupted {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ea80
};

static constexpr Result ResultFsErrorStorageFull {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4eb00
};

static constexpr Result ResultFsErrorJournalFull {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4eb80
};

static constexpr Result ResultFsErrorUnavailableCmd {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ec00
};

static constexpr Result ResultFsErrorUnsupportedCmd {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ec80
};

static constexpr Result ResultFsErrorInvalidParam {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ed00
};

static constexpr Result ResultFsErrorInvalidPath {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ed80
};

static constexpr Result ResultFsErrorInvalidBuffer {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ee00
};

static constexpr Result ResultFsErrorInvalidAlignment {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ee80
};

static constexpr Result ResultFsErrorInvalidClientHandle {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ef00
};

static constexpr Result ResultFsErrorInvalidFileHandle {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4ef80
};

static constexpr Result ResultFsErrorInvalidDirHandle {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f000
};

static constexpr Result ResultFsErrorNotFile {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f080
};

static constexpr Result ResultFsErrorNotDir {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f100
};

static constexpr Result ResultFsErrorFileTooBig {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f180
};

static constexpr Result ResultFsErrorOutOfRange {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f200
};

static constexpr Result ResultFsErrorOutOfResources {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f280
};

static constexpr Result ResultFsErrorMediaNotReady {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f300
};

static constexpr Result ResultFsErrorMediaError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f380
};

static constexpr Result ResultFsErrorWriteProtected {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f400
};

static constexpr Result ResultFsErrorUnknown {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x4f480
};

static constexpr Result ResultFail {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5aa00
};

static constexpr Result ResultMemoryAllocateError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5dc00
};

static constexpr Result ResultInitializeError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e100
};

static constexpr Result ResultSslInitializeError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e180
};

static constexpr Result ResultAcpInitializeError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e200
};

static constexpr Result ResultActInitializeError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e280
};

static constexpr Result ResultPdmInitializeError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e300
};

static constexpr Result ResultConfigInitializeError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e380
};

static constexpr Result ResultFsInitializeError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e400
};

static constexpr Result ResultHTTPInitializeError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e480
};

static constexpr Result ResultAcInitializeError {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x5e500
};

static constexpr Result ResultUnexpect {
   Result::MODULE_NN_BOSS, Result::LEVEL_STATUS, 0x7ff80
};

} // namespace nn::boss
