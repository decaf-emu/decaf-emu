#pragma once
#include "kernel/kernel_hlemodule.h"

namespace nn
{

namespace olv
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual void initialise() override;

   void initialiseUploadedDataBase();
   void initialiseUploadedPostData();

public:
   static void RegisterFunctions();

private:
   static void registerInitFunctions();
   static void registerDownloadedCommunityData();
   static void registerDownloadedTopicData();
   static void registerUploadedDataBase();
   static void registerUploadedPostData();
};

} // namespace olv

} // namespace nn
