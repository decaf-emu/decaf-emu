#include "config.h"
#include "libdecaf/decaf_config.h"
#include <climits>
#include <fstream>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <common/cerealjsonoptionalinput.h>

namespace config
{

namespace log
{

bool async = false;
bool to_file = false;
bool to_stdout = true;
std::string level = "debug";

} // namespace log

namespace system
{

uint32_t timeout_ms = 0;

} // namespace system

struct CerealDebugger
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace decaf::config::debugger;
      ar(CEREAL_NVP(enabled),
         CEREAL_NVP(break_on_entry));
   }
};

struct CerealLog
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace config::log;
      using namespace decaf::config::log;
      ar(CEREAL_NVP(async),
         CEREAL_NVP(to_file),
         CEREAL_NVP(to_stdout),
         CEREAL_NVP(kernel_trace),
         CEREAL_NVP(level));
   }
};

struct CerealJit
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace decaf::config::jit;
      ar(CEREAL_NVP(enabled),
         CEREAL_NVP(verify),
         CEREAL_NVP(cache_size_mb),
         CEREAL_NVP(opt_flags),
         CEREAL_NVP(rodata_read_only));
   }
};

struct CerealSound
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace decaf::config::sound;
      ar(CEREAL_NVP(dump_sounds));
   }
};

struct CerealSystem
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace system;
      using namespace decaf::config::system;
      ar(CEREAL_NVP(region),
         CEREAL_NVP(mlc_path),
         CEREAL_NVP(sdcard_path),
         CEREAL_NVP(timeout_ms));
   }
};

bool load(const std::string &path)
{
   std::ifstream file(path, std::ios::binary);

   if (!file.is_open()) {
      // Create a default config file
      save(path);
      return false;
   }

   try {
      cereal::JSONOptionalInputArchive input(file);
      input(cereal::make_nvp("jit", CerealJit {}),
            cereal::make_nvp("log", CerealLog {}),
            cereal::make_nvp("sound", CerealSound {}),
            cereal::make_nvp("system", CerealSystem {}));
   } catch (std::exception e) {
      // Can't use gLog because it is NULL here.
      std::cout << "Failed to parse " << path << ": " << e.what() << std::endl;
      std::cout << "Try deleting " << path << " to allow a new correct one to be generated with default settings." << std::endl;
   }

   return true;
}

void save(const std::string &path)
{
   std::ofstream file(path, std::ios::binary);
   cereal::JSONOutputArchive output(file);
   output(cereal::make_nvp("jit", CerealJit {}),
          cereal::make_nvp("log", CerealLog {}),
          cereal::make_nvp("sound", CerealSound {}),
          cereal::make_nvp("system", CerealSystem {}));
}

} // namespace config
