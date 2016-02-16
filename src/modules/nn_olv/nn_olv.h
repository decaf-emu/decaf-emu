#pragma once
#include "kernelmodule.h"

namespace nn
{

namespace olv
{

class Module : public KernelModuleImpl<Module>
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
