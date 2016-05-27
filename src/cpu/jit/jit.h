#pragma once
#include "../cpu.h"

namespace cpu
{
namespace jit
{

void initialise();

void clearCache();
void resume(Core *core);

}
}

void fallbacksPrint();
