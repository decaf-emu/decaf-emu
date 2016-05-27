#pragma once
#include "../cpu.h"

namespace cpu
{
namespace interpreter
{

void initialise();

void step_one(Core *core);
void resume(Core *core);

}
}