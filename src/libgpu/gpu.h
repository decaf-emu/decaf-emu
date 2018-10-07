#pragma once
#include <cstdint>

namespace gpu
{

using FlipCallbackFn = void(*)();
using SyncRegisterCallbackFn = void(*)(const uint32_t *registers, uint32_t size);

void
setFlipCallback(FlipCallbackFn callback);

void
setSyncRegisterCallback(SyncRegisterCallbackFn callback);

} // namespace gpu
