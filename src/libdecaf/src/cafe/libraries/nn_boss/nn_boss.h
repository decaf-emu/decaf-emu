#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_boss
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_boss, "nn_boss.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerAccountSymbols();
   void registerAlmightyStorageSymbols();
   void registerAlmightyTaskSymbols();
   void registerClientSymbols();
   void registerDataNameSymbols();
   void registerNbdlTaskSettingSymbols();
   void registerNetTaskSettingSymbols();
   void registerPlayLogUploadTaskSettingSymbols();
   void registerPlayReportSettingSymbols();
   void registerPrivilegedTaskSymbols();
   void registerRawUlTaskSettingSymbols();
   void registerStorageSymbols();
   void registerTaskSymbols();
   void registerTaskIdSymbols();
   void registerTaskSettingSymbols();
   void registerTitleSymbols();
   void registerTitleIdSymbols();
};

} // namespace cafe::nn_boss
