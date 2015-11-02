#pragma once
#include "../cpu.h"

namespace cpu
{
namespace jit
{

void initialise();

void clearCache();
void executeSub(ThreadState *state);

}
}

void fallbacksPrint();
