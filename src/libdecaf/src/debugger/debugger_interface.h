#pragma once
#include <libcpu/state.h>

namespace debugger
{

class DebuggerInterface
{
public:
   virtual bool paused() = 0;
   virtual cpu::Core *getPauseContext(unsigned core) = 0;
   virtual unsigned getPauseInitiator() = 0;

   virtual void pause() = 0;
   virtual void resume() = 0;

   virtual void stepInto(unsigned core) = 0;
   virtual void stepOver(unsigned core) = 0;

   virtual bool hasBreakpoint(uint32_t address) = 0;
   virtual void addBreakpoint(uint32_t address) = 0;
   virtual void removeBreakpoint(uint32_t address) = 0;
};

} // namespace debugger
