#include "coreinit.h"
#include "coreinit_event.h"
#include "coreinit_thread.h"
#include "system.h"
#include "processor.h"

void
OSInitEvent(EventHandle handle, bool value, EventMode mode)
{
   OSInitEventEx(handle, value, mode, nullptr);
}

void
OSInitEventEx(EventHandle handle, bool value, EventMode mode, char *name)
{
   auto event = gSystem.addSystemObject<Event>(handle);
   event->mode = mode;
   event->value = value;
   event->name = name;
}

void
OSSignalEvent(EventHandle handle)
{
   auto event = gSystem.getSystemObject<Event>(handle);
   std::unique_lock<std::mutex> lock { event->mutex };

   event->value = TRUE;

   if (event->mode == EventMode::ManualReset) {
      // Wake all waiting fibers
      for (auto fiber : event->queue) {
         gProcessor.queue(fiber);
      }

      event->queue.clear();
   } else if (event->mode == EventMode::AutoReset) {
      // Wake highest priority fiber
      Fiber *highest = nullptr;

      for (auto fiber : event->queue) {
         if (!highest || fiber->thread->basePriority > highest->thread->basePriority) {
            highest = fiber;
         }
      }

      if (highest) {
         gProcessor.queue(highest);
         event->queue.erase(std::remove(event->queue.begin(), event->queue.end(), highest), event->queue.end());
      }
   }
}

void
OSSignalEventAll(EventHandle handle)
{
   auto event = gSystem.getSystemObject<Event>(handle);
   std::unique_lock<std::mutex> lock { event->mutex };
   event->value = TRUE;

   // Wake all fibers
   for (auto fiber : event->queue) {
      gProcessor.queue(fiber);
   }

   event->queue.clear();
}

void
OSResetEvent(EventHandle handle)
{
   auto event = gSystem.getSystemObject<Event>(handle);
   std::unique_lock<std::mutex> lock { event->mutex };
   event->value = FALSE;
}

void
OSWaitEvent(EventHandle handle)
{
   auto event = gSystem.getSystemObject<Event>(handle);
   auto fiber = gProcessor.getCurrentFiber();

   while (!event->value) {
      std::unique_lock<std::mutex> lock { event->mutex };

      if (event->value) {
         break;
      }

      event->queue.push_back(fiber);
      gProcessor.wait(lock);
   }

   if (event->mode == EventMode::AutoReset) {
      event->value = FALSE;
   }
}

BOOL
OSWaitEventWithTimeout(EventHandle handle, Time timeout)
{
   auto event = gSystem.getSystemObject<Event>(handle);
   auto fiber = gProcessor.getCurrentFiber();
   auto start = std::chrono::system_clock::now();

   while (!event->value) {
      std::unique_lock<std::mutex> lock { event->mutex };

      if (event->value) {
         break;
      }

      // Check for timeout
      auto diff = std::chrono::system_clock::now() - start;
      auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count();

      if (ns >= timeout) {
         return FALSE;
      }

      event->queue.push_back(fiber);
      gProcessor.wait(lock);
   }

   if (event->mode == EventMode::AutoReset) {
      event->value = FALSE;
   }

   return TRUE;
}

void
CoreInit::registerEventFunctions()
{
   RegisterKernelFunction(OSInitEvent);
   RegisterKernelFunction(OSInitEventEx);
   RegisterKernelFunction(OSSignalEvent);
   RegisterKernelFunction(OSSignalEventAll);
   RegisterKernelFunction(OSResetEvent);
   RegisterKernelFunction(OSWaitEvent);
   RegisterKernelFunction(OSWaitEventWithTimeout);
}
