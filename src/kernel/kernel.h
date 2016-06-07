#pragma once
#include "cpu/cpu.h"
#include "platform/platform_fiber.h"

class TeenyHeap;

namespace coreinit
{
struct OSThread;
}

namespace kernel
{

struct Fiber;

void initialise();
void set_game_name(const std::string& name);

void exitThreadNoLock();

TeenyHeap *
getSystemHeap();

void
switchThread(coreinit::OSThread *previous, coreinit::OSThread *next);

}