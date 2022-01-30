#pragma once
#include "nn/nn_result.h"

namespace nn::ipc
{

static constexpr Result ResultNotImplemented {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x3200
};

static constexpr Result ResultInvalidIpcID {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0xfa00
};

static constexpr Result ResultInvalidSessionID {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0xfa80
};

static constexpr Result ResultInvalidIpcHeader {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x6400
};

static constexpr Result ResultInvalidIpcHeaderFormat {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x9600
};

static constexpr Result ResultInvalidIpcHeaderSize {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0xc800
};

static constexpr Result ResultInvalidModuleID {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0xff00
};

static constexpr Result ResultInvalidMethodTag {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x10400
};

static constexpr Result ResultInvalidIpcBuffer {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x12c00
};

static constexpr Result ResultInvalidIpcBufferLength {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x12c80
};

static constexpr Result ResultAccessDenied {
   Result::MODULE_NN_IPC, Result::LEVEL_STATUS, 0x15e00
};

static constexpr const auto ResultCapabilityFailed =
   ResultRange<Result::MODULE_NN_IPC, Result::LEVEL_FATAL, 0x15e80, 0x16d00>();

static constexpr Result ResultDevelopmentOnly {
   Result::MODULE_NN_IPC, Result::LEVEL_STATUS, 0x16d00
};

static constexpr Result ResultUnusualControlFlow {
   Result::MODULE_NN_IPC, Result::LEVEL_STATUS, 0x1f400
};

} // namespace nn::ipc
