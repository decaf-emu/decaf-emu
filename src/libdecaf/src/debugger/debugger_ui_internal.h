#pragma once
#include "modules/coreinit/coreinit_thread.h"

#define HEXTOF(h) static_cast<float>(h&0xFF)/255.0f
#define HEXTOIMV4(h, a) ImVec4(HEXTOF(h>>16), HEXTOF(h>>8), HEXTOF(h>>0), a)

namespace debugger
{

namespace ui
{

bool
isPaused();

bool
hasBreakpoint(uint32_t address);

void
toggleBreakpoint(uint32_t address);

uint64_t
getResumeCount();

coreinit::OSThread *
getActiveThread();

void
setActiveThread(coreinit::OSThread *thread);

cpu::CoreRegs *
getThreadCoreRegs(coreinit::OSThread *thread);

uint32_t
getThreadNia(coreinit::OSThread *thread);

uint32_t
getThreadStack(coreinit::OSThread *thread);

namespace InfoView {
void draw();
}

namespace StatsView {
void draw();
}

namespace DisasmView {
void displayAddress(uint32_t address);
void draw();
}

namespace SegView {
void draw();
}

namespace ThreadView {
void draw();
}

namespace MemView {
void displayAddress(uint32_t address);
void draw();
}

namespace RegView {
void draw();
}

namespace StackView {
void displayAddress(uint32_t address);
void draw();
}

} // namespace ui

} // namespace debugger
