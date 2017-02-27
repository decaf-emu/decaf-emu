#pragma once
#include <atomic>
#include <cstdint>

namespace coreinit
{

namespace internal
{

struct IdLock
{
   std::atomic<uint32_t> owner { 0 };
};

void
acquireIdLock(IdLock &lock,
              uint32_t id);

void
releaseIdLock(IdLock &lock,
              uint32_t id);

void
acquireIdLock(IdLock &lock,
              void *owner);

void
releaseIdLock(IdLock &lock,
              void *owner);

void
acquireIdLock(IdLock &lock);

void
releaseIdLock(IdLock &lock);

} // namespace internal

} // namespace coreinit
