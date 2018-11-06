#pragma once
#include "nn_ipc_command.h"
#include <libcpu/be2_struct.h>

namespace nn::ipc
{

using ServiceId = int;
using ServiceCommandHandler = void(*)(CommandId id);

template<int Id>
struct Service
{
   static constexpr ServiceId id = Id;
};

} // namespace nn::ipc
