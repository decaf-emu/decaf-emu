#include "ios_kernel_hardware.h"
#include "ios_kernel_messagequeue.h"
#include "ios_kernel_scheduler.h"
#include "ios_kernel_process.h"
#include "ios_kernel_thread.h"
#include "ios/ios_core.h"

#include <atomic>

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

struct StaticData
{
   be2_array<EventHandler, 0x30> eventHandlers;
};

static phys_ptr<StaticData>
sData;

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

static void
clearAhbAll(AHBALL mask)
{
   LT_INTSR_AHBALL_ARM &= ~mask.value;
}

static void
clearAndDisableAhbAll(AHBALL mask)
{
   LT_INTSR_AHBALL_ARM &= ~mask.value;
   LT_INTMR_AHBALL_ARM &= ~mask.value;
}

static void
clearAndDisableAhbLt(AHBLT mask)
{
   LT_INTSR_AHBLT_ARM &= ~mask.value;
   LT_INTSR_AHBLT_ARM &= ~mask.value;
}

static void
clearAndEnableAhbAll(AHBALL mask)
{
   // Let's not clear on Clear&Enable to avoid race conditions.
   // This can lead to spurious wakeups but it's better than not waking up.
   // LT_INTSR_AHBALL_ARM &= ~mask.value;
   LT_INTMR_AHBALL_ARM |= mask.value;
}

static void
clearAndEnableAhbLt(AHBLT mask)
{
   // Let's not clear on Clear&Enable to avoid race conditions.
   // This can lead to spurious wakeups but it's better than not waking up.
   // LT_INTSR_AHBALL_ARM &= ~mask.value;
   LT_INTMR_AHBLT_ARM |= mask.value;
}

Error
IOS_HandleEvent(DeviceId id,
                MessageQueueId qid,
                Message message)
{
   phys_ptr<MessageQueue> queue;
   auto error = internal::getMessageQueue(qid, &queue);
   if (error < Error::OK) {
      return error;
   }

   if (id >= sData->eventHandlers.size()) {
      return Error::Invalid;
   }

   queue->flags |= MessageQueueFlags::RegisteredEventHandler;

   auto &handler = sData->eventHandlers[id];
   handler.message = message;
   handler.queue = queue;
   handler.pid = internal::getCurrentProcessId();

   return Error::OK;
}

Error
IOS_ClearAndEnable(DeviceId id)
{
   auto thread = internal::getCurrentThread();

   switch (id) {
   case DeviceId::Timer:
      clearAndEnableAhbAll(AHBALL::get(0).Timer(true));
      break;
   case DeviceId::NandInterfaceAHBALL:
      clearAndEnableAhbAll(AHBALL::get(0).NandInterface(true));
      break;
   case DeviceId::AesEngineAHBALL:
      decaf_check(thread->pid == 3);
      clearAndEnableAhbAll(AHBALL::get(0).AesEngine(true));
      break;
   case DeviceId::Sha1EngineAHBALL:
      decaf_check(thread->pid == 3);
      clearAndEnableAhbAll(AHBALL::get(0).Sha1Engine(true));
      break;
   case DeviceId::UsbEhci:
      clearAndEnableAhbAll(AHBALL::get(0).UsbEhci(true));
      break;
   case DeviceId::UsbOhci0:
      clearAndEnableAhbAll(AHBALL::get(0).UsbOhci0(true));
      break;
   case DeviceId::UsbOhci1:
      clearAndEnableAhbAll(AHBALL::get(0).UsbOhci1(true));
      break;
   case DeviceId::SdHostController:
      clearAndEnableAhbAll(AHBALL::get(0).SdHostController(true));
      break;
   case DeviceId::Wireless80211:
      clearAndEnableAhbAll(AHBALL::get(0).Wireless80211(true));
      break;
   case 9:
      // TODO: latte gpio int flag = 1
      // TODO: latte gpio int mask |= 1
      break;
   case DeviceId::SysProt:
      decaf_check(thread->pid == 0);
      clearAndEnableAhbAll(AHBALL::get(0).SysProt(true));
      break;
   case DeviceId::PowerButton:
      clearAndEnableAhbAll(AHBALL::get(0).PowerButton(true));
      break;
   case DeviceId::DriveInterface:
      clearAndEnableAhbAll(AHBALL::get(0).DriveInterface(true));
      break;
   case DeviceId::ExiRtc:
      clearAndEnableAhbAll(AHBALL::get(0).ExiRtc(true));
      break;
   case DeviceId::Sata:
      clearAndEnableAhbAll(AHBALL::get(0).Sata(true));
      break;
   case DeviceId::IpcStarbuckCompat:
      decaf_check(thread->pid == 0);
      clearAndEnableAhbAll(AHBALL::get(0).IpcStarbuckCompat(true));
      break;
   case DeviceId::Unknown30:
      clearAndEnableAhbLt(AHBLT::get(0).Unknown0(true));
      break;
   case DeviceId::Unknown31:
      clearAndEnableAhbLt(AHBLT::get(0).Unknown1(true));
      break;
   case DeviceId::Unknown32:
      clearAndEnableAhbLt(AHBLT::get(0).Unknown2(true));
      break;
   case DeviceId::Unknown33:
      clearAndEnableAhbLt(AHBLT::get(0).Unknown3(true));
      break;
   case DeviceId::Drh:
      clearAndEnableAhbLt(AHBLT::get(0).Drh(true));
      break;
   case DeviceId::Unknown35:
      clearAndEnableAhbLt(AHBLT::get(0).Unknown5(true));
      break;
   case DeviceId::Unknown36:
      clearAndEnableAhbLt(AHBLT::get(0).Unknown6(true));
      break;
   case DeviceId::Unknown37:
      clearAndEnableAhbLt(AHBLT::get(0).Unknown7(true));
      break;
   case DeviceId::AesEngineAHBLT:
      decaf_check(thread->pid == 3);
      clearAndEnableAhbLt(AHBLT::get(0).AesEngine(true));
      break;
   case DeviceId::Sha1EngineAHBLT:
      decaf_check(thread->pid == 3);
      clearAndEnableAhbLt(AHBLT::get(0).Sha1Engine(true));
      break;
   case DeviceId::Unknown40:
      clearAndEnableAhbLt(AHBLT::get(0).Unknown10(true));
      break;
   case DeviceId::Unknown41:
      clearAndEnableAhbLt(AHBLT::get(0).Unknown11(true));
      break;
   case DeviceId::Unknown42:
      clearAndEnableAhbLt(AHBLT::get(0).Unknown12(true));
      break;
   case DeviceId::I2CEspresso:
      clearAndEnableAhbLt(AHBLT::get(0).I2CEspresso(true));
      break;
   case DeviceId::I2CStarbuck:
      clearAndEnableAhbLt(AHBLT::get(0).I2CStarbuck(true));
      break;
   case DeviceId::IpcStarbuckCore2:
      decaf_check(thread->pid == 0);
      clearAndEnableAhbLt(AHBLT::get(0).IpcStarbuckCore2(true));
      break;
   case DeviceId::IpcStarbuckCore1:
      decaf_check(thread->pid == 0);
      clearAndEnableAhbLt(AHBLT::get(0).IpcStarbuckCore1(true));
      break;
   case DeviceId::IpcStarbuckCore0:
      decaf_check(thread->pid == 0);
      clearAndEnableAhbLt(AHBLT::get(0).IpcStarbuckCore0(true));
      break;
   default:
      return Error::Invalid;
   }

   return Error::OK;
}

namespace internal
{

/**
 * Sends a message to the device event handler message queue.
 *
 * We cannot use IOS_SendMessage due to different scheduling requirements here.
 */
static void
sendEventHandlerMessageNoLock(DeviceId id)
{
   auto &handler = sData->eventHandlers[id];
   auto queue = handler.queue;
   if (queue && queue->used < queue->size) {
      auto index = (queue->first + queue->used) % queue->size;
      queue->messages[index] = handler.message;
      queue->used++;

      internal::wakeupOneThreadNoLock(phys_addrof(queue->receiveQueue),
                                      Error::OK);
   }
}

/**
 * Checks any pending interrupts for LT_INTSR_AHBALL_ARM and LT_INTSR_AHBLT_ARM.
 */
void
handleAhbInterrupts()
{
   internal::lockScheduler();
   auto ahbAll = AHBALL::get(LT_INTSR_AHBALL_ARM.load() &
                             LT_INTMR_AHBALL_ARM.load());

   if (ahbAll.Timer()) {
      clearAhbAll(AHBALL::get(0).Timer(true));
      sendEventHandlerMessageNoLock(DeviceId::Timer);
   }

   if (ahbAll.NandInterface()) {
      clearAndDisableAhbAll(AHBALL::get(0).NandInterface(true));
      sendEventHandlerMessageNoLock(DeviceId::NandInterfaceAHBALL);
   }

   if (ahbAll.AesEngine()) {
      clearAndDisableAhbAll(AHBALL::get(0).AesEngine(true));
      sendEventHandlerMessageNoLock(DeviceId::AesEngineAHBALL);
   }

   if (ahbAll.Sha1Engine()) {
      clearAndDisableAhbAll(AHBALL::get(0).Sha1Engine(true));
      sendEventHandlerMessageNoLock(DeviceId::Sha1EngineAHBALL);
   }

   if (ahbAll.UsbEhci()) {
      clearAndDisableAhbAll(AHBALL::get(0).UsbEhci(true));
      sendEventHandlerMessageNoLock(DeviceId::UsbEhci);
   }

   if (ahbAll.UsbOhci0()) {
      clearAndDisableAhbAll(AHBALL::get(0).UsbOhci0(true));
      sendEventHandlerMessageNoLock(DeviceId::UsbOhci0);
   }

   if (ahbAll.UsbOhci1()) {
      clearAndDisableAhbAll(AHBALL::get(0).UsbOhci1(true));
      sendEventHandlerMessageNoLock(DeviceId::UsbOhci1);
   }

   if (ahbAll.SdHostController()) {
      clearAndDisableAhbAll(AHBALL::get(0).SdHostController(true));
      sendEventHandlerMessageNoLock(DeviceId::SdHostController);
   }

   if (ahbAll.Wireless80211()) {
      clearAndDisableAhbAll(AHBALL::get(0).Wireless80211(true));
      sendEventHandlerMessageNoLock(DeviceId::Wireless80211);
   }

   if (ahbAll.SysProt()) {
      clearAndDisableAhbAll(AHBALL::get(0).SysProt(true));
      sendEventHandlerMessageNoLock(DeviceId::SysProt);
   }

   if (ahbAll.PowerButton()) {
      clearAndDisableAhbAll(AHBALL::get(0).PowerButton(true));
      sendEventHandlerMessageNoLock(DeviceId::PowerButton);
   }

   if (ahbAll.DriveInterface()) {
      clearAndDisableAhbAll(AHBALL::get(0).DriveInterface(true));
      sendEventHandlerMessageNoLock(DeviceId::DriveInterface);
   }

   if (ahbAll.ExiRtc()) {
      clearAndDisableAhbAll(AHBALL::get(0).ExiRtc(true));
      sendEventHandlerMessageNoLock(DeviceId::ExiRtc);
   }

   if (ahbAll.Sata()) {
      clearAndDisableAhbAll(AHBALL::get(0).Sata(true));
      sendEventHandlerMessageNoLock(DeviceId::Sata);
   }

   if (ahbAll.IpcStarbuckCompat()) {
      clearAndDisableAhbAll(AHBALL::get(0).IpcStarbuckCompat(true));
      sendEventHandlerMessageNoLock(DeviceId::IpcStarbuckCompat);
   }

   auto ahbLatte = AHBLT::get(LT_INTSR_AHBLT_ARM.load() &
                              LT_INTMR_AHBLT_ARM.load());

   if (ahbLatte.Unknown0()) {
      clearAndDisableAhbLt(AHBLT::get(0).Unknown0(true));
      sendEventHandlerMessageNoLock(DeviceId::Unknown30);
   }

   if (ahbLatte.Unknown1()) {
      clearAndDisableAhbLt(AHBLT::get(0).Unknown1(true));
      sendEventHandlerMessageNoLock(DeviceId::Unknown31);
   }

   if (ahbLatte.Unknown2()) {
      clearAndDisableAhbLt(AHBLT::get(0).Unknown2(true));
      sendEventHandlerMessageNoLock(DeviceId::Unknown32);
   }

   if (ahbLatte.Unknown3()) {
      clearAndDisableAhbLt(AHBLT::get(0).Unknown3(true));
      sendEventHandlerMessageNoLock(DeviceId::Unknown33);
   }

   if (ahbLatte.Drh()) {
      clearAndDisableAhbLt(AHBLT::get(0).Drh(true));
      sendEventHandlerMessageNoLock(DeviceId::Drh);
   }

   if (ahbLatte.Unknown5()) {
      clearAndDisableAhbLt(AHBLT::get(0).Unknown5(true));
      sendEventHandlerMessageNoLock(DeviceId::Unknown35);
   }

   if (ahbLatte.Unknown6()) {
      clearAndDisableAhbLt(AHBLT::get(0).Unknown6(true));
      sendEventHandlerMessageNoLock(DeviceId::Unknown36);
   }

   if (ahbLatte.Unknown7()) {
      clearAndDisableAhbLt(AHBLT::get(0).Unknown7(true));
      sendEventHandlerMessageNoLock(DeviceId::Unknown37);
   }

   if (ahbLatte.AesEngine()) {
      clearAndDisableAhbLt(AHBLT::get(0).AesEngine(true));
      sendEventHandlerMessageNoLock(DeviceId::AesEngineAHBLT);
   }

   if (ahbLatte.Sha1Engine()) {
      clearAndDisableAhbLt(AHBLT::get(0).Sha1Engine(true));
      sendEventHandlerMessageNoLock(DeviceId::Sha1EngineAHBLT);
   }

   if (ahbLatte.Unknown10()) {
      clearAndDisableAhbLt(AHBLT::get(0).Unknown10(true));
      sendEventHandlerMessageNoLock(DeviceId::Unknown40);
   }

   if (ahbLatte.Unknown11()) {
      clearAndDisableAhbLt(AHBLT::get(0).Unknown11(true));
      sendEventHandlerMessageNoLock(DeviceId::Unknown41);
   }

   if (ahbLatte.Unknown12()) {
      clearAndDisableAhbLt(AHBLT::get(0).Unknown12(true));
      sendEventHandlerMessageNoLock(DeviceId::Unknown42);
   }

   if (ahbLatte.I2CEspresso()) {
      clearAndDisableAhbLt(AHBLT::get(0).I2CEspresso(true));
      sendEventHandlerMessageNoLock(DeviceId::I2CEspresso);
   }

   if (ahbLatte.I2CStarbuck()) {
      clearAndDisableAhbLt(AHBLT::get(0).I2CStarbuck(true));
      sendEventHandlerMessageNoLock(DeviceId::I2CStarbuck);
   }

   if (ahbLatte.IpcStarbuckCore0()) {
      clearAndDisableAhbLt(AHBLT::get(0).IpcStarbuckCore0(true));
      sendEventHandlerMessageNoLock(DeviceId::IpcStarbuckCore0);
   }

   if (ahbLatte.IpcStarbuckCore1()) {
      clearAndDisableAhbLt(AHBLT::get(0).IpcStarbuckCore1(true));
      sendEventHandlerMessageNoLock(DeviceId::IpcStarbuckCore1);
   }

   if (ahbLatte.IpcStarbuckCore2()) {
      clearAndDisableAhbLt(AHBLT::get(0).IpcStarbuckCore2(true));
      sendEventHandlerMessageNoLock(DeviceId::IpcStarbuckCore2);
   }

   internal::rescheduleSelfNoLock();
   internal::unlockScheduler();
}

void
setInterruptAhbAll(AHBALL mask)
{
   LT_INTSR_AHBALL_ARM |= mask.value;
   ios::internal::interrupt(InterruptFlags::Ahb);
}

void
setInterruptAhbLt(AHBLT mask)
{
   LT_INTSR_AHBLT_ARM |= mask.value;
   ios::internal::interrupt(InterruptFlags::Ahb);
}

void
setAlarm(std::chrono::steady_clock::time_point when)
{
}

void
initialiseStaticHardwareData()
{
   sData = allocProcessStatic<StaticData>();
}

} // namespace internal

} // namespace ios
