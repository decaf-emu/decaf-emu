#pragma once
#include "cpu/cpu.h"

namespace cpu
{
namespace jit
{

void initialise();

void clearCache();
void executeSub(ThreadState *state);

}
}