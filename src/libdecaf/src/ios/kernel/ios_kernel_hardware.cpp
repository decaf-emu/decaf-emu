#include "ios_kernel_hardware.h"
#include "ios_kernel_messagequeue.h"
#include "ios_kernel_scheduler.h"
#include "ios_kernel_process.h"
#include "ios_kernel_thread.h"

#include <atomic>
#include <condition_variable>
#include <thread>
#include <mutex>

namespace ios::kernel
{

struct EventHandler
{
   be2_phys_ptr<MessageQueue> queue;
   be2_val<Message> message;
   be2_val<ProcessId> pid;
   PADDING(0x4);
};
CHECK_OFFSET(EventHandler, 0x00, queue);
CHECK_OFFSET(EventHandler, 0x04, message);
CHECK_OFFSET(EventHandler, 0x08, pid);
CHECK_SIZE(EventHandler, 0x10);

struct StaticHardwareData
{
   be2_array<EventHandler, 0x30> eventHandlers;
   be2_val<BOOL> bspReady;
};

static phys_ptr<StaticHardwareData>
sHardwareData;

//! Interrupt mask for ARM AHB IRQs
static std::atomic<uint32_t>
LT_INTMR_AHBALL_ARM { 0 };

//! Triggered interrupts for ARM AHB IRQs
static std::atomic<uint32_t>
LT_INTSR_AHBALL_ARM { 0 };

//! Interrupt mask for latte ARM AHB IRQs
static std::atomic<uint32_t>
LT_INTMR_AHBLT_ARM { 0 };

//! Triggered interrupts for latte ARM AHB IRQs
static std::atomic<uint32_t>
LT_INTSR_AHBLT_ARM { 0 };

static std::thread sHardwareThread;
static std::condition_variable sHardwareConditionVariable;
static std::mutex sHardwareMutex;
static std::atomic<bool> sRunning;

/**
 * Registers a message queue as the event handler for a device.
 *
 * Sends the given message to the message queue when an interrupt is received
 * for the given device.
 */
Error
IOS_HandleEvent(DeviceId id,
                MessageQueueId qid,
                Message message)
{
   phys_ptr<MessageQueue> queue;
   auto error = internal::getMessageQueueSafe(qid, &queue);
   if (error < Error::OK) {
      return error;
   }

   if (id >= sHardwareData->eventHandlers.size()) {
      return Error::Invalid;
   }

   queue->flags |= MessageQueueFlags::RegisteredEventHandler;

   auto &handler = sHardwareData->eventHandlers[id];
   handler.message = message;
   handler.queue = queue;
   handler.pid = internal::getCurrentProcessId();

   return Error::OK;
}


/**
 * Unregister an event handler previously registered by IOS_HandleEvent.
 */
Error
IOS_UnregisterEventHandler(DeviceId id)
{
   if (id >= sHardwareData->eventHandlers.size()) {
      return Error::Invalid;
   }

   auto &handler = sHardwareData->eventHandlers[id];

   if (handler.queue) {
      handler.queue->flags &= ~MessageQueueFlags::RegisteredEventHandler;
   }

   std::memset(phys_addrof(handler).get(), 0, sizeof(handler));
   return Error::OK;
}


/**
 * Clear and enable interrupts for given DeviceId.
 */
Error
IOS_ClearAndEnable(DeviceId id)
{
   auto thread = internal::getCurrentThread();

   // We don't actually clear the signalling register so we can avoid race
   // conditions with signals coming from external threads. Instead we just
   // enable the mask, which could lead to spurious wakeups but they are much
   // better than not waking up at all.
   auto enableAhbAll =
      [](AHBALL mask) {
         LT_INTMR_AHBALL_ARM |= mask.value;
      };

   auto enableAhbLatte =
      [](AHBLT mask) {
         LT_INTMR_AHBLT_ARM |= mask.value;
      };

   switch (id) {
   case DeviceId::Timer:
      enableAhbAll(AHBALL::get(0).Timer(true));
      break;
   case DeviceId::NandInterfaceAHBALL:
      enableAhbAll(AHBALL::get(0).NandInterface(true));
      break;
   case DeviceId::AesEngineAHBALL:
      decaf_check(thread->pid == 3);
      enableAhbAll(AHBALL::get(0).AesEngine(true));
      break;
   case DeviceId::Sha1EngineAHBALL:
      decaf_check(thread->pid == 3);
      enableAhbAll(AHBALL::get(0).Sha1Engine(true));
      break;
   case DeviceId::UsbEhci:
      enableAhbAll(AHBALL::get(0).UsbEhci(true));
      break;
   case DeviceId::UsbOhci0:
      enableAhbAll(AHBALL::get(0).UsbOhci0(true));
      break;
   case DeviceId::UsbOhci1:
      enableAhbAll(AHBALL::get(0).UsbOhci1(true));
      break;
   case DeviceId::SdHostController:
      enableAhbAll(AHBALL::get(0).SdHostController(true));
      break;
   case DeviceId::Wireless80211:
      enableAhbAll(AHBALL::get(0).Wireless80211(true));
      break;
   case 9:
      // TODO: latte gpio int flag = 1
      // TODO: latte gpio int mask |= 1
      break;
   case DeviceId::SysProt:
      decaf_check(thread->pid == 0);
      enableAhbAll(AHBALL::get(0).SysProt(true));
      break;
   case DeviceId::PowerButton:
      enableAhbAll(AHBALL::get(0).PowerButton(true));
      break;
   case DeviceId::DriveInterface:
      enableAhbAll(AHBALL::get(0).DriveInterface(true));
      break;
   case DeviceId::ExiRtc:
      enableAhbAll(AHBALL::get(0).ExiRtc(true));
      break;
   case DeviceId::Sata:
      enableAhbAll(AHBALL::get(0).Sata(true));
      break;
   case DeviceId::IpcStarbuckCompat:
      decaf_check(thread->pid == 0);
      enableAhbAll(AHBALL::get(0).IpcStarbuckCompat(true));
      break;
   case DeviceId::Unknown30:
      enableAhbLatte(AHBLT::get(0).SdHostController(true));
      break;
   case DeviceId::Unknown31:
      enableAhbLatte(AHBLT::get(0).Unknown1(true));
      break;
   case DeviceId::Unknown32:
      enableAhbLatte(AHBLT::get(0).Unknown2(true));
      break;
   case DeviceId::Unknown33:
      enableAhbLatte(AHBLT::get(0).Unknown3(true));
      break;
   case DeviceId::Drh:
      enableAhbLatte(AHBLT::get(0).Drh(true));
      break;
   case DeviceId::Unknown35:
      enableAhbLatte(AHBLT::get(0).Unknown5(true));
      break;
   case DeviceId::Unknown36:
      enableAhbLatte(AHBLT::get(0).Unknown6(true));
      break;
   case DeviceId::Unknown37:
      enableAhbLatte(AHBLT::get(0).Unknown7(true));
      break;
   case DeviceId::AesEngineAHBLT:
      decaf_check(thread->pid == 3);
      enableAhbLatte(AHBLT::get(0).AesEngine(true));
      break;
   case DeviceId::Sha1EngineAHBLT:
      decaf_check(thread->pid == 3);
      enableAhbLatte(AHBLT::get(0).Sha1Engine(true));
      break;
   case DeviceId::Unknown40:
      enableAhbLatte(AHBLT::get(0).Unknown10(true));
      break;
   case DeviceId::Unknown41:
      enableAhbLatte(AHBLT::get(0).Unknown11(true));
      break;
   case DeviceId::Unknown42:
      enableAhbLatte(AHBLT::get(0).Unknown12(true));
      break;
   case DeviceId::I2CEspresso:
      enableAhbLatte(AHBLT::get(0).I2CEspresso(true));
      break;
   case DeviceId::I2CStarbuck:
      enableAhbLatte(AHBLT::get(0).I2CStarbuck(true));
      break;
   case DeviceId::IpcStarbuckCore2:
      decaf_check(thread->pid == 0);
      enableAhbLatte(AHBLT::get(0).IpcStarbuckCore2(true));
      break;
   case DeviceId::IpcStarbuckCore1:
      decaf_check(thread->pid == 0);
      enableAhbLatte(AHBLT::get(0).IpcStarbuckCore1(true));
      break;
   case DeviceId::IpcStarbuckCore0:
      decaf_check(thread->pid == 0);
      enableAhbLatte(AHBLT::get(0).IpcStarbuckCore0(true));
      break;
   default:
      return Error::Invalid;
   }

   return Error::OK;
}


/**
 * Set BSP ready flag used for boot.
 */
Error
IOS_SetBspReady()
{
   sHardwareData->bspReady = TRUE;
   return Error::OK;
}


namespace internal
{

/**
 * Get BSP ready flag.
 */
bool
bspReady()
{
   return !!sHardwareData->bspReady;
}


/**
 * Sends a message to the device event handler message queue.
 *
 * We cannot use IOS_SendMessage due to different scheduling requirements here.
 */
static void
sendEventHandlerMessageNoLock(DeviceId id)
{
   auto &handler = sHardwareData->eventHandlers[id];
   auto queue = handler.queue;
   if (queue && queue->used < queue->size) {
      auto index = (queue->first + queue->used) % queue->size;
      queue->messages[index] = handler.message;
      queue->used++;

      internal::wakeupOneThread(phys_addrof(queue->receiveQueue), Error::OK);
   }
}

void
setInterruptAhbAll(AHBALL mask)
{
   auto lock = std::unique_lock { sHardwareMutex };
   LT_INTSR_AHBALL_ARM |= mask.value;
   sHardwareConditionVariable.notify_all();
}

void
setInterruptAhbLt(AHBLT mask)
{
   auto lock = std::unique_lock { sHardwareMutex };
   LT_INTSR_AHBLT_ARM |= mask.value;
   sHardwareConditionVariable.notify_all();
}

void
unregisterEventHandlerQueue(MessageQueueId queue)
{
   for (auto &handler : sHardwareData->eventHandlers) {
      if (handler.queue->uid == queue) {
         std::memset(phys_addrof(handler).get(), 0, sizeof(handler));
         break;
      }
   }
}

void
initialiseStaticHardwareData()
{
   LT_INTMR_AHBALL_ARM.store(AHBALL::get(0)
                             .Timer(true)
                             .LatteGpioStarbuck(true)
                             .PowerButton(true)
                             .value);
   LT_INTSR_AHBALL_ARM.store(0);
   LT_INTMR_AHBLT_ARM.store(0);
   LT_INTSR_AHBLT_ARM.store(0);
   sHardwareData = allocProcessStatic<StaticHardwareData>();
}

/**
 * Send a message to the queues registered with IOS_HandleEvent for the events
 * indicated by the AHBALL and AHBLT register values.
 */
static void
handleEvents(AHBALL ahbAll,
             AHBLT ahbLatte)
{
   if (ahbAll.Timer()) {
      sendEventHandlerMessageNoLock(DeviceId::Timer);
   }

   if (ahbAll.NandInterface()) {
      sendEventHandlerMessageNoLock(DeviceId::NandInterfaceAHBALL);
   }

   if (ahbAll.AesEngine()) {
      sendEventHandlerMessageNoLock(DeviceId::AesEngineAHBALL);
   }

   if (ahbAll.Sha1Engine()) {
      sendEventHandlerMessageNoLock(DeviceId::Sha1EngineAHBALL);
   }

   if (ahbAll.UsbEhci()) {
      sendEventHandlerMessageNoLock(DeviceId::UsbEhci);
   }

   if (ahbAll.UsbOhci0()) {
      sendEventHandlerMessageNoLock(DeviceId::UsbOhci0);
   }

   if (ahbAll.UsbOhci1()) {
      sendEventHandlerMessageNoLock(DeviceId::UsbOhci1);
   }

   if (ahbAll.SdHostController()) {
      sendEventHandlerMessageNoLock(DeviceId::SdHostController);
   }

   if (ahbAll.Wireless80211()) {
      sendEventHandlerMessageNoLock(DeviceId::Wireless80211);
   }

   if (ahbAll.SysProt()) {
      sendEventHandlerMessageNoLock(DeviceId::SysProt);
   }

   if (ahbAll.PowerButton()) {
      sendEventHandlerMessageNoLock(DeviceId::PowerButton);
   }

   if (ahbAll.DriveInterface()) {
      sendEventHandlerMessageNoLock(DeviceId::DriveInterface);
   }

   if (ahbAll.ExiRtc()) {
      sendEventHandlerMessageNoLock(DeviceId::ExiRtc);
   }

   if (ahbAll.Sata()) {
      sendEventHandlerMessageNoLock(DeviceId::Sata);
   }

   if (ahbAll.IpcStarbuckCompat()) {
      sendEventHandlerMessageNoLock(DeviceId::IpcStarbuckCompat);
   }

   if (ahbLatte.SdHostController()) {
      sendEventHandlerMessageNoLock(DeviceId::Unknown30);
   }

   if (ahbLatte.Unknown1()) {
      sendEventHandlerMessageNoLock(DeviceId::Unknown31);
   }

   if (ahbLatte.Unknown2()) {
      sendEventHandlerMessageNoLock(DeviceId::Unknown32);
   }

   if (ahbLatte.Unknown3()) {
      sendEventHandlerMessageNoLock(DeviceId::Unknown33);
   }

   if (ahbLatte.Drh()) {
      sendEventHandlerMessageNoLock(DeviceId::Drh);
   }

   if (ahbLatte.Unknown5()) {
      sendEventHandlerMessageNoLock(DeviceId::Unknown35);
   }

   if (ahbLatte.Unknown6()) {
      sendEventHandlerMessageNoLock(DeviceId::Unknown36);
   }

   if (ahbLatte.Unknown7()) {
      sendEventHandlerMessageNoLock(DeviceId::Unknown37);
   }

   if (ahbLatte.AesEngine()) {
      sendEventHandlerMessageNoLock(DeviceId::AesEngineAHBLT);
   }

   if (ahbLatte.Sha1Engine()) {
      sendEventHandlerMessageNoLock(DeviceId::Sha1EngineAHBLT);
   }

   if (ahbLatte.Unknown10()) {
      sendEventHandlerMessageNoLock(DeviceId::Unknown40);
   }

   if (ahbLatte.Unknown11()) {
      sendEventHandlerMessageNoLock(DeviceId::Unknown41);
   }

   if (ahbLatte.Unknown12()) {
      sendEventHandlerMessageNoLock(DeviceId::Unknown42);
   }

   if (ahbLatte.I2CEspresso()) {
      sendEventHandlerMessageNoLock(DeviceId::I2CEspresso);
   }

   if (ahbLatte.I2CStarbuck()) {
      sendEventHandlerMessageNoLock(DeviceId::I2CStarbuck);
   }

   if (ahbLatte.IpcStarbuckCore0()) {
      sendEventHandlerMessageNoLock(DeviceId::IpcStarbuckCore0);
   }

   if (ahbLatte.IpcStarbuckCore1()) {
      sendEventHandlerMessageNoLock(DeviceId::IpcStarbuckCore1);
   }

   if (ahbLatte.IpcStarbuckCore2()) {
      sendEventHandlerMessageNoLock(DeviceId::IpcStarbuckCore2);
   }
}

#if 0
/**
 * Check and handle any pending interrupts.
 *
 * To be called from kernel scheduler to ensure we do not miss any interrupts
 * in the case that we always have new kernel threads to run.
 */
void
checkAndHandleInterrupts()
{
   // Check for pending interrupts
   auto lock = std::unique_lock { sHardwareMutex };

   // Read unmasked interrupts
   auto ahbLatte = LT_INTSR_AHBLT_ARM & LT_INTMR_AHBLT_ARM;
   auto ahbAll = LT_INTSR_AHBALL_ARM & LT_INTMR_AHBALL_ARM;

   // Clear and disable handled interrupts
   LT_INTMR_AHBLT_ARM &= ~ahbLatte;
   LT_INTSR_AHBLT_ARM &= ~ahbLatte;

   LT_INTMR_AHBALL_ARM &= ~ahbAll;
   LT_INTSR_AHBALL_ARM &= ~ahbAll;

   if (ahbLatte || ahbAll) {
      lock.unlock();
      handleEvents(AHBALL::get(ahbAll), AHBLT::get(ahbLatte));
   }
}
#endif

static void
hardwareThreadEntry()
{
   setIdleFiber();

   while (sRunning) {
      // Check for any pending threads to run
      reschedule();

      // Check for pending interrupts
      auto lock = std::unique_lock { sHardwareMutex };

      // Read unmasked interrupts
      auto ahbLatte = LT_INTSR_AHBLT_ARM & LT_INTMR_AHBLT_ARM;
      auto ahbAll = LT_INTSR_AHBALL_ARM & LT_INTMR_AHBALL_ARM;

      // Clear and disable handled interrupts
      LT_INTMR_AHBLT_ARM &= ~ahbLatte;
      LT_INTSR_AHBLT_ARM &= ~ahbLatte;

      LT_INTMR_AHBALL_ARM &= ~ahbAll;
      LT_INTSR_AHBALL_ARM &= ~ahbAll;

      if (ahbLatte || ahbAll) {
         lock.unlock();
         handleEvents(AHBALL::get(ahbAll), AHBLT::get(ahbLatte));
      } else if (sRunning) {
         sHardwareConditionVariable.wait(lock);
      }
   }
}

void
startHardwareThread()
{
   sRunning = true;
   sHardwareThread = std::thread { hardwareThreadEntry };
}

void
joinHardwareThread()
{
   sRunning = false;
   sHardwareConditionVariable.notify_all();
   sHardwareThread.join();
}

} // namespace internal

} // namespace ios
