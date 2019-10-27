#pragma once
#include "nn/nn_result.h"

namespace nn::ipc
{

static constexpr Result ResultNOT_IMPLEMENTED {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x3200
};

static constexpr Result ResultINVALID_IPC_ID {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0xfa00
};

static constexpr Result ResultINVALID_SESSION_ID {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0xfa80
};

static constexpr Result ResultINVALID_IPC_HEADER {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x6400
};

static constexpr Result ResultINVALID_IPC_HEADER_FORMAT {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x9600
};

static constexpr Result ResultINVALID_IPC_HEADER_SIZE {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0xc800
};

static constexpr Result ResultINVALID_MODULE_ID {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0xff00
};

static constexpr Result ResultINVALID_METHOD_TAG {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x10400
};

static constexpr Result ResultINVALID_IPC_BUFFER {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x12c00
};

static constexpr Result ResultINVALID_IPC_BUFFER_LENGTH {
   Result::MODULE_NN_IPC, Result::LEVEL_USAGE, 0x12c80
};

static constexpr Result ResultACCESS_DENIED {
   Result::MODULE_NN_IPC, Result::LEVEL_STATUS, 0x15e00
};

static constexpr const auto ResultCAPABILITY_FAILED =
   ResultRange<Result::MODULE_NN_IPC, Result::LEVEL_FATAL, 0x15e80, 0x16d00>();

static constexpr Result ResultDEVELOPMENT_ONLY {
   Result::MODULE_NN_IPC, Result::LEVEL_STATUS, 0x16d00
};

static constexpr Result ResultUNUSUAL_CONTROL_FLOW {
   Result::MODULE_NN_IPC, Result::LEVEL_STATUS, 0x1f400
};


} // namespace nn::ipc
