#include <functional>
#include "debugger.h"
#include "log.h"
#include "processor.h"
#include "system.h"

Debugger
gDebugger;

enum class DebugPacketType : uint32_t {
   Invalid,
   PreLaunch,
   BpHit,
};

class DebugPacket {
public:
   virtual DebugPacketType type() const = 0;

};

class DebugPacketBpHit : public DebugPacket {
public:
   virtual DebugPacketType type() const override { return DebugPacketType::BpHit; }

   uint32_t address;
   uint32_t coreId;
   uint32_t userData;

};

class DebugPacketPreLaunch : public DebugPacket {
public:
   virtual DebugPacketType type() const override { return DebugPacketType::PreLaunch; }

};

Debugger::Debugger()
{
   mWaitingForPause.store(false);
   mNumBreakpoints.store(0);

   for (int i = 0; i < CoreCount; ++i) {
      mCoreStopped[i] = false;
      mStates[i] = nullptr;
   }
}

void
Debugger::addMessage(DebugPacket *msg)
{
   std::unique_lock<std::mutex> lock{ mMsgLock };
   mMsgQueue.push(msg);
   mMsgCond.notify_all();
}

void
Debugger::debugThread()
{
   printf("Debugger Thread Started");

   while (true) {
      std::unique_lock<std::mutex> lock{ mMsgLock };
      if (mMsgQueue.empty()) {
         mMsgCond.wait(lock);
         continue;
      }

      DebugPacket *pak = mMsgQueue.front();
      mMsgQueue.pop();
      lock.unlock();

      switch (pak->type()) {
      case DebugPacketType::PreLaunch: {
         auto xpak = reinterpret_cast<DebugPacketPreLaunch*>(pak);

         waitForAllPaused();

         gLog->debug("Prelaunch Occured");

         auto userMod = gSystem.getUserModule();
         auto preinitUserEntry = gMemory.untranslate(userMod->findExport("__preinit_user"));
         auto applicationEntry = userMod->entryPoint();

         if (preinitUserEntry) {
            addBreakpoint(preinitUserEntry, 0);
         }
         if (applicationEntry) {
            addBreakpoint(applicationEntry, 0);
         }

         resumeAll();
         break;
      }
      case DebugPacketType::BpHit: {
         auto xpak = reinterpret_cast<DebugPacketBpHit*>(pak);

         waitForAllPaused();

         gLog->debug("Breakpoint Hit @ {:08x} on Core #{}", xpak->address, xpak->coreId);

         resumeAll();
         break;
      }
      }

      delete pak;
   }
}

void
Debugger::initialise()
{
   mConnected = false;

   mDebuggerThread = std::thread(&Debugger::debugThread, this);
   
   std::string address = "127.0.0.1";
   uint16_t port = 11234;

   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   
   struct sockaddr_in server;
   server.sin_addr.s_addr = inet_addr(address.c_str());
   server.sin_family = AF_INET;
   server.sin_port = htons(port);

   int conres = ::connect(sock, (struct sockaddr *)&server, sizeof(server));
   if (conres < 0) {
      // Connect failure
      return;
   }

   mConnected = true;
}

void
Debugger::preLaunch()
{
   if (hasDebugger()) {
      pauseAll();
      addMessage(new DebugPacketPreLaunch());
      pauseCore(nullptr, OSGetCoreId());
      return;
   }
}

bool
Debugger::hasDebugger() const
{
   return true;
}

void
Debugger::addBreakpoint(uint32_t addr, uint32_t userData)
{
   std::unique_lock<std::mutex> lock{ mMutex };
   mBreakpoints.emplace(addr, userData);
   mNumBreakpoints.store(mBreakpoints.size());
}

void
Debugger::removeBreakpoint(uint32_t addr)
{
   std::unique_lock<std::mutex> lock{ mMutex };
   auto bpitr = mBreakpoints.find(addr);
   if (bpitr != mBreakpoints.end()) {
      mBreakpoints.erase(bpitr);
   }
   mNumBreakpoints.store(mBreakpoints.size());
}

void
Debugger::pauseAll()
{
   bool prevVal = mWaitingForPause.exchange(true);
   if (prevVal == true) {
      // Someone pre-empted this pause
      return;
   }

   gProcessor.wakeAll();
}

void
Debugger::resumeAll()
{
   for (int i = 0; i < CoreCount; ++i) {
      mCoreStopped[i] = false;
      mStates[i] = nullptr;
   }

   mWaitingForPause.store(false);

   mReleaseCond.notify_all();
}


void
Debugger::waitForAllPaused()
{
   std::unique_lock<std::mutex> lock{ mMutex };

   while (true) {
      bool allCoresStopped = true;
      for (int i = 0; i < CoreCount; ++i) {
         allCoresStopped &= mCoreStopped[i];
      }
      if (allCoresStopped) {
         break;
      }

      mWaitCond.wait(lock);
   }
}

void
Debugger::pauseCore(ThreadState *state, uint32_t coreId)
{
   while (mWaitingForPause.load()) {
      std::unique_lock<std::mutex> lock{ mMutex };

      mStates[coreId] = state;
      mCoreStopped[coreId] = true;
      mWaitCond.notify_all();

      mReleaseCond.wait(lock);
   }
}

void
Debugger::maybeBreak(uint32_t addr, ThreadState *state, uint32_t coreId)
{
   if (mWaitingForPause.load()) {
      pauseCore(state, coreId);
      return;
   }

   uint32_t bpUserData = 0;
   bool isBpAddr = false;
   if (mNumBreakpoints.load() > 0) {
      // TODO: Somehow make this lockless...
      std::unique_lock<std::mutex> lockX{ mMutex };
      auto bpitr = mBreakpoints.find(addr);
      if (bpitr != mBreakpoints.end()) {
         bpUserData = bpitr->second;
         isBpAddr = true;
      }
   }

   if (isBpAddr) {
      pauseAll();
      
      // Send a message to the debugger thread before we pause ourself
      auto pak = new DebugPacketBpHit();
      pak->address = addr;
      pak->coreId = coreId;
      pak->userData = bpUserData;
      addMessage(pak);

      pauseCore(state, coreId);
      return;
   }
}