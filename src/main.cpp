#include <pugixml.hpp>
#include "bitutils.h"
#include "codetests.h"
#include "filesystem.h"
#include "instructiondata.h"
#include "interpreter.h"
#include "jit.h"
#include "loader.h"
#include "log.h"
#include "memory.h"
#include "modules/coreinit/coreinit.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/gx2/gx2.h"
#include "modules/nn_save/nn_save.h"
#include "modules/zlib125/zlib125.h"
#include "system.h"
#include "thread.h"
#include "usermodule.h"

int main(int argc, char **argv)
{
   if (argc < 2) {
      xLog() << "Usage: " << argv[0] << " <play|test> <options...>";
      return -1;
   }

   // Parse command line
   std::string testDirectory;
   std::string assemblerPath;
   std::string gamePath;
   std::string command = argv[1];

   if (command.compare("play") == 0) {
      if (argc < 3) {
         xLog() << "Usage: " << argv[0] << " play <game directory>";
         return -1;
      }

      gamePath = argv[2];
   } else if (command.compare("test") == 0) {
      if (argc < 3) {
         xLog() << "Usage: " << argv[0] << " test [<powerpc-eabi-as.exe>] <test directory>";
         return -1;
      }

      if (argc >= 4) {
         assemblerPath = argv[2];
         testDirectory = argv[3];
      } else if (argc >= 3) {
         assemblerPath = "powerpc-eabi-as.exe";
         testDirectory = argv[2];
      }
   } else {
      xLog() << "Usage: " << argv[0] << " <play|test> <options...>";
      return -1;
   }

   // Static module initialisation
   Interpreter::RegisterFunctions();
   JitManager::RegisterFunctions();
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

   // If user used test command, execute tests
   if (assemblerPath.size() && testDirectory.size()){
      auto testRes = executeCodeTests(assemblerPath, testDirectory);
      xLog() << "";
      system("PAUSE");
      return testRes ? 0 : -1;
   }

   // Setup filesystem
   VirtualFileSystem fs { "/" };
   fs.mount("/vol", std::make_unique<HostFileSystem>(gamePath + "/data"));
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

   xLog() << "Succesfully loaded " << gamePath;

   auto stackSize = entry.stackSize;

   // Allocate OSThread structures and stacks
   OSThread *thread1 = OSAllocFromSystem<OSThread>();
   uint8_t *stack1 = reinterpret_cast<uint8_t*>(OSAllocFromSystem(stackSize, 8));
   uint8_t *stack1top = stack1 + stackSize;

   OSThread *thread0 = OSAllocFromSystem<OSThread>();
   uint8_t *stack0 = reinterpret_cast<uint8_t*>(OSAllocFromSystem(stackSize, 8));
   uint8_t *stack0top = stack0 + stackSize;

   OSThread *thread2 = OSAllocFromSystem<OSThread>();
   uint8_t *stack2 = reinterpret_cast<uint8_t*>(OSAllocFromSystem(stackSize, 8));
   uint8_t *stack2top = stack0 + stackSize;

   // Create the idle threads for core0 and core2
   OSCreateThread(thread0, 0, 0, nullptr, stack0top, stackSize, 16, OSThreadAttributes::AffinityCPU0);
   OSCreateThread(thread2, 0, 0, nullptr, stack2top, stackSize, 16, OSThreadAttributes::AffinityCPU2);

   // Create the main thread for core1
   OSCreateThread(thread1, entry.address, 0, nullptr, stack1top, stackSize, 16, OSThreadAttributes::AffinityCPU1);

   // Set the default threads
   OSSetDefaultThread(0, thread0);
   OSSetDefaultThread(1, thread1);
   OSSetDefaultThread(2, thread2);

   // Start thread 1 and wait for it to finish
   be_val<int> exitValue;
   OSResumeThread(thread1);
   OSJoinThread(thread1, &exitValue);

   // Free allocated data
   OSFreeToSystem(thread0);
   OSFreeToSystem(thread1);
   OSFreeToSystem(thread2);
   OSFreeToSystem(stack0);
   OSFreeToSystem(stack1);
   OSFreeToSystem(stack2);

   xLog() << "";
   system("PAUSE");
   return 0;
}
