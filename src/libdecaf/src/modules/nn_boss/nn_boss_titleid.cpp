#include "nn_boss.h"
#include "nn_boss_titleid.h"

namespace nn
{

namespace boss
{

TitleID::TitleID()
{
   decaf_warn_stub();

   mLower = 0;
   mUpper = 0;
}

TitleID::TitleID(TitleID *other)
{
   decaf_warn_stub();

   mLower = other->mLower;
   mUpper = other->mUpper;
}

TitleID::TitleID(uint64_t id)
{
   decaf_warn_stub();

   mLower = static_cast<uint32_t>(id);
   mUpper = static_cast<uint32_t>(id >> 32);
}

bool
TitleID::IsValid()
{
   decaf_warn_stub();

   return !!(mLower | mUpper);
}

uint64_t
TitleID::GetValue()
{
   decaf_warn_stub();

   return static_cast<uint64_t>(mUpper) << 32 | static_cast<uint64_t>(mLower);
}

uint32_t
TitleID::GetTitleID()
{
   decaf_warn_stub();

   return mUpper;
}

uint32_t
TitleID::GetTitleCode()
{
   decaf_warn_stub();

   return mUpper;
}

uint32_t
TitleID::GetUniqueId()
{
   decaf_warn_stub();

   return ((mUpper << 8) | (mUpper >> 24)) & 0xFFFFF;
}

void
Module::registerTitleIDFunctions()
{
   RegisterKernelFunctionConstructor("__ct__Q3_2nn4boss7TitleIDFv", TitleID);
   RegisterKernelFunctionConstructorArgs("__ct__Q3_2nn4boss7TitleIDFRCQ3_2nn4boss7TitleID", TitleID, TitleID *);
   RegisterKernelFunctionConstructorArgs("__ct__Q3_2nn4boss7TitleIDFUL", TitleID, uint64_t);

   RegisterKernelFunctionName("IsValid__Q3_2nn4boss7TitleIDCFv", &TitleID::IsValid);
   RegisterKernelFunctionName("GetValue__Q3_2nn4boss7TitleIDCFv", &TitleID::GetValue);
   RegisterKernelFunctionName("GetTitleId__Q3_2nn4boss7TitleIDCFv", &TitleID::GetTitleID);
   RegisterKernelFunctionName("GetTitleCode__Q3_2nn4boss7TitleIDCFv", &TitleID::GetTitleCode);
   RegisterKernelFunctionName("GetUniqueId__Q3_2nn4boss7TitleIDCFv", &TitleID::GetUniqueId);
}

} // namespace boss

} // namespace nn
