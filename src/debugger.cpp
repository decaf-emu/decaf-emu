#include <functional>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <sstream>
#include <atomic>
#include <iostream>
#include "debugger.h"
#include "debugmsg.h"
#include "debugnet.h"
#include "debugcontrol.h"
#include "common/log.h"
#include "cpu/cpu.h"
#include "cpu/mem.h"
#include "cpu/espresso/espresso_disassembler.h"
#include "cpu/espresso/espresso_instructionset.h"

static const bool FORCE_DEBUGGER_ON = false;

Debugger
gDebugger;

Debugger::Debugger()
   : mEnabled(false), mPaused(false), mWaitingForStep(-1), mWaitingForPause(false)
{
}

void
Debugger::debugThread()
{
   gLog->info("Debugger Thread Started");

   while (true) {
      std::unique_lock<std::mutex> lock{ mMsgLock };
      if (mMsgQueue.empty()) {
         mMsgCond.wait(lock);
         continue;
      }

      DebugMessage *msg = mMsgQueue.front();
      mMsgQueue.pop();
      lock.unlock();

      handleMessage(msg);
      delete msg;
   }
}

const cpu::Core * Debugger::getCorePauseState(uint32_t coreId) const
{
   return mCorePauseState[coreId];
}

void
Debugger::handleMessage(DebugMessage *msg)
{
   gLog->debug("Handling debug message {}", (int)msg->type());

   switch (msg->type()) {
   case DebugMessageType::DebuggerDc: {
      bool wasAlreadyPaused = mPaused;
      if (!wasAlreadyPaused) {
         gDebugControl.pauseAll();

         // We can do this because this thread blocks above, so all the pause
         // messages from the pause above will still be pending in the queue.
         mWaitingForPause = true;
      }

      gLog->info("Debugger disconnected, game paused to wait for debugger.");

      while (!gDebugNet.connect()) {
         std::this_thread::sleep_for(std::chrono::milliseconds(400));
      }

      if (wasAlreadyPaused) {
         gDebugNet.writePaused();
      }

      break;
   }
   case DebugMessageType::CorePaused: {
      auto bpMsg = reinterpret_cast<DebugMessageCorePaused*>(msg);

      mCorePauseState[bpMsg->coreId] = bpMsg->state;

      if (bpMsg->wasInitiator) {
         mPauseInitiatorCoreId = bpMsg->coreId;
      }

      if (mCorePauseState[0] && mCorePauseState[1] && mCorePauseState[2]) {
         if (mWaitingForPause) {
            gLog->info("Application Paused");
            gDebugNet.writePaused();
         } else if (mWaitingForStep != -1) {
            gLog->info("Core #{} Stepped", mPauseInitiatorCoreId);
            gDebugNet.writeCoreStepped(mPauseInitiatorCoreId);
         } else {
            gLog->info("Breakpoint Hit on Core #{}", mPauseInitiatorCoreId);
            gDebugNet.writeBreakpointHit(mPauseInitiatorCoreId);
         }

         mWaitingForPause = false;
         mWaitingForStep = -1;
         mPaused = true;
      }

      gLog->info("Core #{} Paused", bpMsg->coreId);

      break;
   }
   case DebugMessageType::Invalid:
      gLog->debug("Invalid message");
      break;
   }
}

void
Debugger::initialise()
{
   if (gDebugNet.connect()) {
      // Debugging is connected!
      mEnabled = true;
      gLog->info("Debugger Enabled! Remote");
   } else if (FORCE_DEBUGGER_ON) {
      mEnabled = true;
      gLog->info("Debugger Enabled! Local");
   } else {
      mEnabled = false;
      gLog->info("Debugger Disabled.");
   }

   if (mEnabled) {
      mDebuggerThread = std::thread(&Debugger::debugThread, this);
   }
}

bool
Debugger::isEnabled() const
{
   return mEnabled;
}

void
Debugger::notify(DebugMessage *msg)
{
   assert(mEnabled);

   std::unique_lock<std::mutex> lock{ mMsgLock };
   mMsgQueue.push(msg);
   mMsgCond.notify_all();
}

void
Debugger::pause()
{
   if (mPaused) {
      return;
   }

   mWaitingForPause = true;
   gDebugControl.pauseAll();
}

void
Debugger::resume()
{
   assert(mPaused);

   mPaused = false;
   mCorePauseState[0] = nullptr;
   mCorePauseState[1] = nullptr;
   mCorePauseState[2] = nullptr;
   gDebugControl.resumeAll();
}

void
Debugger::stepCore(uint32_t coreId)
{
   if (!mPaused) {
      return;
   }

   gDebugControl.stepCore(coreId);
}

void
Debugger::stepCoreOver(uint32_t coreId)
{
   if (!mPaused) {
      return;
   }

   auto curAddr = mCorePauseState[coreId]->nia;
   auto instr = mem::read<espresso::Instruction>(curAddr);
   auto data = espresso::decodeInstruction(instr);
   if (data->id == espresso::InstructionID::b
      || data->id == espresso::InstructionID::bc
      || data->id == espresso::InstructionID::bcctr
      || data->id == espresso::InstructionID::bclr) {
      if (instr.lk) {
         // This is a branch-and-link.  Put a BP after the next instruction
         cpu::addBreakpoint(curAddr + 4, cpu::SYSTEM_BPFLAG);
         gDebugger.resume();
      } else {
         // Direct branch, just step
         gDebugControl.stepCore(coreId);
      }
   } else {
      // Not a branch, just step
      gDebugControl.stepCore(coreId);
   }
}

void
Debugger::clearBreakpoints()
{
   assert(mEnabled);
   cpu::clearBreakpoints(cpu::USER_BPFLAG);
}

void
Debugger::addBreakpoint(uint32_t addr)
{
   assert(mEnabled);
   cpu::addBreakpoint(addr, cpu::USER_BPFLAG);
}

void
Debugger::removeBreakpoint(uint32_t addr)
{
   assert(mEnabled);
   cpu::removeBreakpoint(addr, cpu::USER_BPFLAG);
}
