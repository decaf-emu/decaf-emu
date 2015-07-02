#include <pugixml.hpp>
#include <docopt.h>
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

void initialiseEmulator();
bool test(const std::string &as, const std::string &path);
bool play(const std::string &path);

static const char USAGE[] =
   R"(WiiU Emulator

       Usage:
         wiiu play <game directory>
         wiiu test [--as=<ppcas>] <test directory>
         wiiu (-h | --help)
         wiiu --version

       Options:
         -h --help     Show this screen.
         --version     Show version.
         --as=<ppcas>  Path to PowerPC assembler [default: powerpc-eabi-as.exe].
   )";

int main(int argc, char **argv)
{
   auto args = docopt::docopt(USAGE, { argv + 1, argv + argc }, true, "WiiU 0.1");
   bool result = false;

   initialiseEmulator();

   if (args["play"]) {
      result = play(args["<game directory>"].asString());
   } else if (args["test"]) {
      result = test(args["--as"].asString(), args["<test directory>"].asString());
   }

   system("PAUSE");
   return result ? 0 : -1;
}

static void
initialiseEmulator()
{
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
}

static bool
test(const std::string &as, const std::string &path)
{   return executeCodeTests(as, path);
}

static bool
play(const std::string &path)
{
   // Setup filesystem
   VirtualFileSystem fs { "/" };
   fs.mount("/vol", std::make_unique<HostFileSystem>(path + "/data"));
   gSystem.setFileSystem(&fs);

   // Read cos.xml
   pugi::xml_document doc;
   auto cosFile = fs.openFile("/vol/code/cos.xml", FileSystem::Input | FileSystem::Binary);

   if (!cosFile) {
      xError() << "Error opening /vol/code/cos.xml";
      return false;
   }

   auto size = cosFile->size();
   auto buffer = std::vector<char>(size);
   cosFile->read(buffer.data(), size);

   // Parse cos.xml
   auto parseResult = doc.load_buffer_inplace(buffer.data(), buffer.size());

   if (!parseResult) {
      xError() << "Error parsing /vol/code/cos.xml";
      return false;
   }

   auto rpxPath = doc.child("app").child("argstr").child_value();

   // Read rpx file
   auto rpxFile = fs.openFile(std::string("/vol/code/") + rpxPath, FileSystem::Input | FileSystem::Binary);

   if (!rpxFile) {
      xError() << "Error opening /vol/code/" << rpxPath;
      return false;
   }

   buffer.resize(rpxFile->size());
   rpxFile->read(buffer.data(), buffer.size());

   // Load the rpl into memory
   Loader loader;
   UserModule module;
   gSystem.registerModule(rpxPath, &module);

   if (!loader.loadRPL(module, buffer.data(), buffer.size())) {
      xError() << "Could not load elf file";
      return false;
   }

   xLog() << "Succesfully loaded " << path;
   auto entryPoint = module.entryPoint;
   auto stackSize = module.defaultStackSize;

   // Allocate OSThread structures and stacks
   auto thread1 = OSAllocFromSystem<OSThread>();
   auto stack1 = reinterpret_cast<uint8_t*>(OSAllocFromSystem(stackSize, 8));
   auto stack1top = stack1 + stackSize;

   auto thread0 = OSAllocFromSystem<OSThread>();
   auto stack0 = reinterpret_cast<uint8_t*>(OSAllocFromSystem(stackSize, 8));
   auto stack0top = stack0 + stackSize;

   auto thread2 = OSAllocFromSystem<OSThread>();
   auto stack2 = reinterpret_cast<uint8_t*>(OSAllocFromSystem(stackSize, 8));
   auto stack2top = stack0 + stackSize;

   // Create the idle threads for core0 and core2
   OSCreateThread(thread0, 0, 0, nullptr, stack0top, stackSize, 16, OSThreadAttributes::AffinityCPU0);
   OSCreateThread(thread2, 0, 0, nullptr, stack2top, stackSize, 16, OSThreadAttributes::AffinityCPU2);

   // Create the main thread for core1
   OSCreateThread(thread1, entryPoint, 0, nullptr, stack1top, stackSize, 16, OSThreadAttributes::AffinityCPU1);

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

   return exitValue == 0;
}
