#pragma once
#include "../cpu.h"
#include "../espresso/espresso_instructionid.h"

namespace cpu
{
namespace jit
{

void initialise();

void clearCache();
void resume();

bool hasInstruction(espresso::InstructionID instrId);

}
}
