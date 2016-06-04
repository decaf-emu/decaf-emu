#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <thread>
#include <queue>
#include "debugmsg.h"
#include "modules/coreinit/coreinit_core.h"
#include "cpu/cpu.h"

class DebugPacket;
class DebugMessage;

typedef std::map<uint32_t, uint32_t> BreakpointListType;
typedef std::shared_ptr<BreakpointListType> BreakpointList;

enum class DebugMessageType : uint16_t {
   Invalid,
   DebuggerDc,
   CorePaused,
};

class DebugMessage : public MessageClass<DebugMessageType> { };
template<DebugMessageType TypeId>
using DebugMessageBase = MessageClassBase<DebugMessage, TypeId>;

class DebugMessageCorePaused : public DebugMessageBase<DebugMessageType::CorePaused> {
public:
   uint32_t coreId;
   bool wasInitiator;
   cpu::Core *state;
};

class DebugMessageDebuggerDc : public DebugMessageBase<DebugMessageType::DebuggerDc> {
public:
};

class Debugger
{
public:
   Debugger();

   void initialise();
   bool isEnabled() const;

   void pause();
   void resume();
   void stepCore(uint32_t coreId);
   void stepCoreOver(uint32_t coreId);

   void clearBreakpoints();
   void addBreakpoint(uint32_t addr);
   void removeBreakpoint(uint32_t addr);

   const cpu::Core * getCorePauseState(uint32_t addr) const;

   void notify(DebugMessage *msg);

protected:
   void handleMessage(DebugMessage *pak);
   void debugThread();

   bool mEnabled;
   bool mPaused;
   bool mWaitingForPause;
   int mWaitingForStep;
   std::thread mDebuggerThread;
   uint32_t mPauseInitiatorCoreId;
   cpu::Core *mCorePauseState[3];

   std::queue<DebugMessage*> mMsgQueue;
   std::mutex mMsgLock;
   std::condition_variable mMsgCond;
};

extern Debugger gDebugger;
