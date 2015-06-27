#include <iostream>
#include <fstream>
#include <vector>
#include <pugixml.hpp>
#include "bitutils.h"
#include "gdbstub.h"
#include "instructiondata.h"
#include "interpreter.h"
#include "loader.h"
#include "log.h"
#include "memory.h"
#include "modules/coreinit/coreinit.h"
#include "modules/gx2/gx2.h"
#include "modules/nn_save/nn_save.h"
#include "modules/zlib125/zlib125.h"
#include "system.h"
#include "thread.h"
#include "usermodule.h"
#include "filesystem.h"

int main(int argc, char **argv)
{
   if (argc < 2) {
      xLog() << "Usage: " << argv[0] << " <extracted game directory>";
      return -1;
   }

   // Static module initialisation
   Interpreter::RegisterFunctions();
   CoreInit::RegisterFunctions();
   GX2::RegisterFunctions();
   NNSave::RegisterFunctions();
   Zlib125::RegisterFunctions();

   // Initialise emulator systems
   gMemory.initialise();
   gInstructionTable.initialise();
   gSystem.registerModule("coreinit", new CoreInit {});
   gSystem.registerModule("gx2", new GX2 {});
   gSystem.registerModule("nn_save", new NNSave {});
   gSystem.registerModule("zlib125", new Zlib125 {});
   gSystem.initialiseModules();

   // Setup filesystem
   VirtualFileSystem fs { "/" };
   fs.mount("/vol", std::make_unique<HostFileSystem>(std::string(argv[1]) + "/data"));
   gSystem.setFileSystem(&fs);

   // Load cos.xml
   pugi::xml_document doc;
   auto cosFile = fs.openFile("/vol/code/cos.xml", FileSystem::Input | FileSystem::Binary);

   if (!cosFile) {
      xError() << "Error opening /vol/code/cos.xml";
      return -1;
   }

   auto size = cosFile->size();
   auto buffer = std::vector<char>(size);
   cosFile->read(buffer.data(), size);

   auto result = doc.load_buffer_inplace(buffer.data(), buffer.size());

   if (!result) {
      xError() << "Error parsing /vol/code/cos.xml";
      return -1;
   }

   auto rpxPath = doc.child("app").child("argstr").child_value();

   // Load rpx file
   auto rpxFile = fs.openFile(std::string("/vol/code/") + rpxPath, FileSystem::Input | FileSystem::Binary);

   if (!rpxFile) {
      xError() << "Error opening /vol/code/" << rpxPath;
      return -1;
   }

   buffer.resize(rpxFile->size());
   rpxFile->read(buffer.data(), buffer.size());

   // Load the elf
   auto loader = Loader {};
   auto module = new UserModule {};
   auto entry = EntryInfo {};
   gSystem.registerModule(rpxPath, module);

   if (!loader.loadRPL(*module, entry, buffer.data(), buffer.size())) {
      xError() << "Could not load elf file";
      return -1;
   }

   xLog() << "Succesfully loaded " << argv[1];

   auto thread = new Thread(&gSystem, entry.stackSize, entry.address);
   thread->run(gInterpreter);

   return 0;
}
