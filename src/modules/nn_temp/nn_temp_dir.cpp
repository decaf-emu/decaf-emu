#include <random>
#include "nn_temp.h"
#include "nn_temp_dir.h"
#include "filesystem/filesystem.h"
#include "system.h"

namespace nn
{

namespace temp
{

static std::random_device
gRandomDevice;

static std::mt19937_64
gMersenne { gRandomDevice() };

static std::uniform_int_distribution<int64_t>
gDistribution { std::llround(std::pow(2, 61)), std::llround(std::pow(2, 62)) };

TempStatus
TEMPCreateAndInitTempDir(uint32_t maxSize,
                         uint32_t pref,
                         be_val<TempDirID> *idOut)
{
   auto fs = gSystem.getFileSystem();
   auto id = gDistribution(gMersenne);

   if (!fs->makeFolder(nn_temp::internal::getTempDir(id))) {
      return TempStatus::FatalError;
   }

   *idOut = id;
   return TempStatus::OK;
}

TempStatus
TEMPShutdownTempDir(TempDirID id)
{
   auto fs = gSystem.getFileSystem();

   if (!fs->deleteFolder(nn_temp::internal::getTempDir(id))) {
      return TempStatus::FatalError;
   }

   return TempStatus::OK;
}

void
Module::registerDirFunctions()
{
   RegisterKernelFunction(TEMPCreateAndInitTempDir);
   RegisterKernelFunction(TEMPShutdownTempDir);
}

namespace internal
{

std::string
getTempDir(TempDirID id)
{
   return fmt::format("/vol/temp/{:016X}", id);
}

} // namespace internal

} // namespace temp

} // namespace nn

