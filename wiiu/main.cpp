#include <iostream>
#include <fstream>
#include <vector>
#include "bitutils.h"
#include "instructiondata.h"
#include "interpreter.h"
#include "loader.h"
#include "log.h"
#include "memory.h"

int main(int argc, char **argv)
{
   if (argc < 2) {
      xLog() << "Usage: " << argv[0] << " *.rpx";
      return -1;
   }

   auto file = std::ifstream { argv[1], std::ifstream::binary };
   auto buffer = std::vector<char> {};

   if (!file.is_open()) {
      xError() << "Could not open file " << argv[1];
   }

   // Get file size
   file.seekg(0, std::istream::end);
   auto size = (unsigned int)file.tellg();
   file.seekg(0, std::istream::beg);

   // Read whole file
   buffer.resize(size);
   file.read(buffer.data(), size);
   assert(file.gcount() == size);

   // Setup our virtual memory
   gMemory.initialise();
   gInstructionTable.initialise();

   // Load the elf
   auto loader = Loader {};
   auto bin = Binary {};

   if (!loader.loadElf(bin, buffer.data(), buffer.size())) {
      xError() << "Could not load elf file";
      return -1;
   }

   xLog() << "Succesfully loaded " << argv[1];

   // Start up cpu!
   ThreadState state;
   memset(&state, 0, sizeof(ThreadState));

   // Setup state
   state.bin = &bin;
   state.cia = bin.header.e_entry;
   state.nia = state.cia + 4;

   auto stackSize = 65536u;
   auto stack = 0x06000000;
   state.gpr[1] = stack + stackSize; // Stack Base

   Interpreter interpreter;
   interpreter.initialise();
   interpreter.execute(&state);

   return 0;
}
