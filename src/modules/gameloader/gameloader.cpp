#include "gameloader.h"
#include <string>
#include "systemtypes.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "system.h"
#include "interpreter.h"

std::string gGameRpx;

GameLoader::GameLoader()
{
}

void
GameLoader::initialise()
{
}

void
GameLoaderInit(const char *rpxName)
{
   gGameRpx = rpxName;
}

void
GameLoaderRun()
{
   auto appModule = gLoader.loadRPL(gGameRpx.c_str());
   if (!appModule) {
      gLog->error("Could not load {}", gGameRpx);
      return;
   }

   gSystem.setUserModule(appModule);

   gLog->debug("Succesfully loaded {}", gGameRpx);

   auto userPreinit = appModule->findFuncExport<void, be_ptr<CommonHeap>*, be_ptr<CommonHeap>*, be_ptr<CommonHeap>*>("__preinit_user");
   if (userPreinit) {
      struct HeapHandles {
         be_ptr<CommonHeap> mem1Heap;
         be_ptr<CommonHeap> fgHeap;
         be_ptr<CommonHeap> mem2Heap;
      };
      HeapHandles *wiiHandles = OSAllocFromSystem<HeapHandles>();

      wiiHandles->mem1Heap = MEMGetBaseHeapHandle(BaseHeapType::MEM1);
      wiiHandles->fgHeap = MEMGetBaseHeapHandle(BaseHeapType::FG);
      wiiHandles->mem2Heap = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
      userPreinit(&wiiHandles->mem1Heap, &wiiHandles->fgHeap, &wiiHandles->mem2Heap);
      MEMSetBaseHeapHandle(BaseHeapType::MEM1, wiiHandles->mem1Heap);
      MEMSetBaseHeapHandle(BaseHeapType::FG, wiiHandles->fgHeap);
      MEMSetBaseHeapHandle(BaseHeapType::MEM2, wiiHandles->mem2Heap);

      OSFreeToSystem(wiiHandles);
   }

   // Create default threads
   for (auto i = 0u; i < CoreCount; ++i) {
      auto thread = OSAllocFromSystem<OSThread>();
      auto stackSize = appModule->defaultStackSize();
      auto stack = reinterpret_cast<uint8_t*>(OSAllocFromSystem(stackSize, 8));
      auto name = OSSprintfFromSystem("Default Thread %d", i);

      OSCreateThread(thread,
         0, 0, nullptr,
         stack + stackSize, stackSize,
         16,
         static_cast<OSThreadAttributes::Flags>(1 << i));

      OSSetDefaultThread(i, thread);
      OSSetThreadName(thread, name);
   }

   // Create interrupt threads
   for (auto i = 0u; i < CoreCount; ++i) {
      auto thread = OSAllocFromSystem<OSThread>();
      auto stackSize = 16 * 1024;
      auto stack = reinterpret_cast<uint8_t*>(OSAllocFromSystem(stackSize, 8));
      auto name = OSSprintfFromSystem("Interrupt Thread %d", i);

      OSCreateThread(thread,
         InterruptThreadEntryPoint, i, nullptr,
         stack + stackSize, stackSize,
         16,
         static_cast<OSThreadAttributes::Flags>(1 << i));

      OSSetInterruptThread(i, thread);
      OSSetThreadName(thread, name);
      OSResumeThread(thread);
   }

   // Run thread 1
   OSRunThread(OSGetDefaultThread(1), appModule->entryPoint(), 0, nullptr);
}

void
GameLoader::RegisterFunctions()
{
   RegisterKernelFunction(GameLoaderRun);
}
