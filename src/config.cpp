#include "config.h"
#include <cereal/archives/json.hpp>
#include <fstream>

namespace config
{

namespace dx12
{
bool use_warp = true;
}

namespace gx2
{
bool dump_textures = false;
bool dump_shaders = false;
}

namespace log
{
bool async = true;
bool to_file = false;
bool to_stdout = true;
bool kernel_trace = true;
std::string filename = "log";
std::string level = "info";
}

namespace jit
{
bool enabled = false;
bool debug = false;
}

namespace system
{
std::string system_path = "/undefined_system_path";
}

struct CerealDX12
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace dx12;
      ar(CEREAL_NVP(use_warp));
   }
};

struct CerealGX2
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace gx2;
      ar(CEREAL_NVP(dump_textures),
         CEREAL_NVP(dump_shaders));
   }
};

struct CerealLog
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace log;
      ar(CEREAL_NVP(async),
         CEREAL_NVP(to_file),
         CEREAL_NVP(to_stdout),
         CEREAL_NVP(kernel_trace),
         CEREAL_NVP(filename),
         CEREAL_NVP(level));
   }
};

struct CerealJit
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace jit;
      ar(CEREAL_NVP(enabled),
         CEREAL_NVP(debug));
   }
};

struct CerealSystem
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace system;
      ar(CEREAL_NVP(system_path));
   }
};

bool load(const std::string &path)
{
   std::ifstream file(path, std::ios::binary);

   if (!file.is_open()) {
      // Create a default config file
      std::ofstream out(path, std::ios::binary);
      cereal::JSONOutputArchive output(out);
      output(cereal::make_nvp("dx12", CerealDX12 {}),
             cereal::make_nvp("gx2", CerealGX2 {}),
             cereal::make_nvp("log", CerealLog {}),
             cereal::make_nvp("jit", CerealJit {}),
             cereal::make_nvp("system", CerealSystem {}));
      return false;
   }

   cereal::JSONInputArchive input(file);
   input(cereal::make_nvp("dx12", CerealDX12 {}),
         cereal::make_nvp("gx2", CerealGX2 {}),
         cereal::make_nvp("log", CerealLog {}),
         cereal::make_nvp("jit", CerealJit {}),
         cereal::make_nvp("system", CerealSystem {}));
   return true;
}

} // namespace config
