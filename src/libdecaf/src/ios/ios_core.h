#pragma once
#include <atomic>
#include <common/platform_fiber.h>
#include <thread>

#include "ios_enum.h"

namespace ios
{

using CoreID = uint32_t;

struct Core
{
   CoreID id;
   std::thread thread;
   platform::Fiber *fiber;
   std::atomic<uint32_t> interruptFlags;
};

namespace internal
{

Core *
getCurrentCore();

CoreID
getCurrentCoreID();

uint32_t
getCoreCount();

void
interruptCore(CoreID core,
              CoreInterruptFlags flags);

void
interrupt(InterruptFlags flags);

} // namespace internal

} // namespace ios
