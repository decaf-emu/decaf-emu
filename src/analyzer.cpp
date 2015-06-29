#include "instructiondata.h"
#include "memory.h"

bool isBranch(Instruction instr)
{
   return isA<InstructionID::b>(instr)
       || isA<InstructionID::bc>(instr)
       || isA<InstructionID::bcctr>(instr)
       || isA<InstructionID::bclr>(instr);
}

void analyze(uint32_t addr)
{
   while (true) {
      Instruction instr = gMemory.read<uint32_t>(addr);

      isA<InstructionID::b>(instr);
      isA<InstructionID::bc>(instr);
      isA<InstructionID::bcctr>(instr);
      isA<InstructionID::bclr>(instr);
   }
}
