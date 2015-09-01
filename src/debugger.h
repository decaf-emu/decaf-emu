#pragma once
#include <map>
#include <mutex>
#include <atomic>
#include <queue>
#include "systemtypes.h"
#include "debugmsg.h"
#include "modules/coreinit/coreinit_core.h"

class DebugPacket;
class DebugMessage;

typedef std::map<uint32_t, uint32_t> BreakpointListType;
typedef std::shared_ptr<BreakpointListType> BreakpointList;

enum class DebugMessageType : uint16_t {
   Invalid,
   PreLaunch,
   DebuggerDc,
   BpHit,
};

class DebugMessage : public MessageClass<DebugMessageType> { };
template<DebugMessageType TypeId>
using DebugMessageBase = MessageClassBase<DebugMessage, TypeId>;

class DebugMessageBpHit : public DebugMessageBase<DebugMessageType::BpHit> {
public:
   uint32_t coreId;
   uint32_t userData;

};

class DebugMessagePreLaunch : public DebugMessageBase<DebugMessageType::PreLaunch> {
public:

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

   void addBreakpoint(uint32_t addr, uint32_t userData);
   void removeBreakpoint(uint32_t addr);
   const BreakpointList& getBreakpoints() const {
      return mBreakpoints;
   }

   void notify(DebugMessage *msg);

protected:
   void handleMessage(DebugMessage *pak);
   void debugThread();

   bool mEnabled;
   std::thread mDebuggerThread;
   BreakpointList mBreakpoints;
   
   std::queue<DebugMessage*> mMsgQueue;
   std::mutex mMsgLock;
   std::condition_variable mMsgCond;

};

extern Debugger gDebugger;
