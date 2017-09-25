#pragma once
#include <atomic>
#include <common/platform_fiber.h>
#include <thread>

#include "ios_enum.h"

namespace ios
{

using CoreId = uint32_t;

struct Core
{
   CoreId id;
   std::thread thread;
   platform::Fiber *fiber;
   std::atomic<uint32_t> interruptFlags;
};

namespace internal
{

Core *
getCurrentCore();

CoreId
getCurrentCoreId();

uint32_t
getCoreCount();

void
interruptCore(CoreId core,
              CoreInterruptFlags flags);

void
interrupt(InterruptFlags flags);

} // namespace internal

} // namespace ios
