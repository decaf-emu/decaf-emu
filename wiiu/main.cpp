#include <iostream>
#include <fstream>
#include <vector>
#include "bitutils.h"
#include "gdbstub.h"
#include "instructiondata.h"
#include "interpreter.h"
#include "loader.h"
#include "log.h"
#include "memory.h"
#include "modules/coreinit/coreinit.h"
#include "modules/gx2/gx2.h"
#include "system.h"

int main(int argc, char **argv)
{
   if (argc < 2) {
      xLog() << "Usage: " << argv[0] << " *.rpx";
      return -1;
   }

   // Initialise emulator systems
   gMemory.initialise();
   gInstructionTable.initialise();
   gSystem.registerModule("coreinit", new CoreInit);
   gSystem.registerModule("gx2", new GX2);

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

   // Load the elf
   auto loader = Loader {};
   auto module = Module {};
   auto entry = EntryInfo {};

   if (!loader.loadRPL(module, entry, buffer.data(), buffer.size())) {
      xError() << "Could not load elf file";
      return -1;
   }

   xLog() << "Succesfully loaded " << argv[1];

   // Start up cpu!
   ThreadState state;
   memset(&state, 0, sizeof(ThreadState));

   // Setup state
   state.module = &module;
   state.cia = entry.address;
   state.nia = state.cia + 4;

   // Setup stack
   auto stack = gMemory.alloc(MemoryType::SystemData, entry.stackSize);
   state.gpr[1] = stack + entry.stackSize;

   Interpreter interpreter;
   interpreter.initialise();
   startGDBStub(&interpreter);
   interpreter.addBreakpoint(entry.address);
   interpreter.execute(&state);

   return 0;
}
