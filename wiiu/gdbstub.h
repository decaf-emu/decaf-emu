#pragma once
#include <cstdint>

struct ThreadState;

class IDebugInterface
{
public:
   virtual ~IDebugInterface()
   {
   }

   virtual void pause() = 0;
   virtual void resume() = 0;
   virtual void step() = 0;
   virtual void addBreakpoint(uint32_t bp) = 0;
   virtual void removeBreakpoint(uint32_t addr) = 0;
   virtual ThreadState *getThreadState(uint32_t tid) = 0;
};

#ifdef GDB_STUB
void startGDBStub(IDebugInterface *debug);
#endif
