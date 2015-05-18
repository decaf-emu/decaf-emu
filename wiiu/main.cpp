#include <iostream>
#include <fstream>
#include <vector>
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

   // Load the elf
   auto loader = Loader {};
   auto bin = Binary {};

   if (!loader.loadElf(bin, buffer.data(), buffer.size())) {
      xError() << "Could not load elf file";
      return -1;
   }

   xLog() << "Succesfully loaded " << argv[1];

   // Start up cpu!
   auto realEntryPoint = gMemory.translate(bin.header.e_entry);
   xLog() << "Starting interpret at " << Log::hex(bin.header.e_entry) << " (" << Log::hex(realEntryPoint) << ")";

   // TODO: The whole emulator

   return 0;
}
