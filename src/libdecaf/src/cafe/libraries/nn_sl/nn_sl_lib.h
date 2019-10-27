#pragma once
#include "nn/nn_result.h"
#include <libcpu/be2_struct.h>

namespace cafe::nn_sl
{

using AllocFn = virt_func_ptr<virt_ptr<void> (uint32_t size, uint32_t align)>;
using FreeFn = virt_func_ptr<void (virt_ptr<void> ptr)>;

struct ITransferrer;
struct DrcTransferrer;

nn::Result
Initialize(AllocFn alloc, FreeFn free);

nn::Result
Initialize(AllocFn alloc, FreeFn free, virt_ptr<ITransferrer> transferrer);

nn::Result
Finalize();

virt_ptr<DrcTransferrer>
GetDrcTransferrer();

}  // namespace namespace cafe::nn_sl
