#pragma once
#include "nn_ec_memorymanager.h"

#include "nn/nn_result.h"
#include <libcpu/be2_struct.h>

namespace cafe::nn_ec
{

nn::Result
Initialize(uint32_t unk0);

nn::Result
Finalize();

nn::Result
SetAllocator(MemoryManager::AllocFn allocate, MemoryManager::FreeFn free);

} // namespace cafe::nn_ec
