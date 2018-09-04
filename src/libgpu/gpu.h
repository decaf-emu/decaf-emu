#pragma once
#include <cstdint>

namespace gpu
{

using FlipCallbackFn = void(*)();
using RetireCallbackFn = void(*)(void *context);
using SyncRegisterCallbackFn = void(*)(const uint32_t *registers, uint32_t size);

void
setFlipCallback(FlipCallbackFn callback);

void
setRetireCallback(RetireCallbackFn callback);

void
setSyncRegisterCallback(SyncRegisterCallbackFn callback);

} // namespace gpu
