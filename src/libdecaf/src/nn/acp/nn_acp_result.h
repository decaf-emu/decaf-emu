#pragma once
#include "nn/nn_result.h"

namespace nn::acp
{

static constexpr Result ResultSuccess {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_SUCCESS, 128
};

static constexpr Result ResultFsInvalid {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_USAGE, 0x6480
};

static constexpr Result ResultFsUnexpectedFileSize {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_USAGE, 0x6500
};

static constexpr Result ResultXmlRootNodeNotFound {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_FATAL, 0x6580
};

static constexpr Result ResultFsAccessError {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_USAGE, 0x6600
};

static constexpr Result ResultFsNotFound {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0xFA00
};

static constexpr Result ResultXmlNodeNotFound {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0xFD80
};

static constexpr Result ResultFsAlreadyExists {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x12C00
};

static constexpr Result ResultFsAlreadyOpen {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x15E00
};

static constexpr Result ResultFsPermissionError {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x22600
};

static constexpr Result ResultFsBusy {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x28A00
};

static constexpr Result ResultFsCancelled {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x2BC00
};

static constexpr Result ResultStorageFull {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x2EE80
};

static constexpr Result ResultJournalFull {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x2EF00
};

static constexpr Result ResultFsEnd {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x2F000
};

static constexpr Result ResultFsNotInit {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x32000
};

static constexpr Result ResultDataCorrupted {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x3E880
};

static constexpr Result ResultSlcDataCorrupted {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x3E900
};

static constexpr Result ResultMlcDataCorrupted {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x3E980
};

static constexpr Result ResultUsbDataCorrupted {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x3EA00
};

static constexpr Result ResultMediaNotReady {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x41A80
};

static constexpr Result ResultMediaError {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x41B00
};

static constexpr Result ResultOddMediaNotReady {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x41B80
};

static constexpr Result ResultOddMediaError {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x41C00
};

static constexpr Result ResultUsbMediaNotReady {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x41C80
};

static constexpr Result ResultUsbMediaError {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x41D00
};

static constexpr Result ResultWriteProtected {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x41D80
};

static constexpr Result ResultUsbWriteProtected {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 0x41E00
};

static constexpr Result ResultFatalFilesystemError {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_FATAL, 0x7D080
};

} // namespace nn::acp
