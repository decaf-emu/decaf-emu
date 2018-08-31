#pragma once
#include <libcpu/pointer.h>

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

namespace ringbuffer
{

void
submit(void *context,
       uint32_t *buffer,
       uint32_t numWords);

} // namespace ringbuffer

} // namespace gpu
