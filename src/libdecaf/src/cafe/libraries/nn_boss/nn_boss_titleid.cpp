#include "nn_boss.h"
#include "nn_boss_titleid.h"

namespace cafe::nn::boss
{

TitleID::TitleID() :
   mLower(uint32_t { 0 }),
   mUpper(uint32_t { 0 })
{
}

TitleID::TitleID(virt_ptr<TitleID> other) :
   mLower(other->mLower),
   mUpper(other->mUpper)
{
}

TitleID::TitleID(uint64_t id) :
   mLower(static_cast<uint32_t>(id)),
   mUpper(static_cast<uint32_t>(id >> 32))
{
}

bool
TitleID::IsValid()
{
   return !!(mLower | mUpper);
}

uint64_t
TitleID::GetValue()
{
   return static_cast<uint64_t>(mUpper) << 32 | static_cast<uint64_t>(mLower);
}

uint32_t
TitleID::GetTitleID()
{
   return mUpper;
}

uint32_t
TitleID::GetTitleCode()
{
   return GetTitleID();
}

uint32_t
TitleID::GetUniqueId()
{
   return (mUpper >> 8) & 0xFFFFFu;
}

bool
TitleID::operator ==(virt_ptr<TitleID> other)
{
   if (mLower & 0x20) {
      return (((mLower ^ other->mLower) | (mUpper ^ other->mUpper)) & 0xFFFFFF00) == 0;
   } else {
      return (mLower == other->mLower) && (mUpper == other->mUpper);
   }
}

bool
TitleID::operator !=(virt_ptr<TitleID> other)
{
   return !(*this == other);
}

void
Library::registerTitleIdSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn4boss7TitleIDFv",
                             TitleID);
   RegisterConstructorExportArgs("__ct__Q3_2nn4boss7TitleIDFRCQ3_2nn4boss7TitleID",
                                 TitleID, virt_ptr<TitleID>);
   RegisterConstructorExportArgs("__ct__Q3_2nn4boss7TitleIDFUL",
                                 TitleID, uint64_t);

   RegisterFunctionExportName("IsValid__Q3_2nn4boss7TitleIDCFv",
                              &TitleID::IsValid);
   RegisterFunctionExportName("GetValue__Q3_2nn4boss7TitleIDCFv",
                              &TitleID::GetValue);
   RegisterFunctionExportName("GetTitleId__Q3_2nn4boss7TitleIDCFv",
                              &TitleID::GetTitleID);
   RegisterFunctionExportName("GetTitleCode__Q3_2nn4boss7TitleIDCFv",
                              &TitleID::GetTitleCode);
   RegisterFunctionExportName("GetUniqueId__Q3_2nn4boss7TitleIDCFv",
                              &TitleID::GetUniqueId);
   RegisterFunctionExportName("__eq__Q3_2nn4boss7TitleIDCFRCQ3_2nn4boss7TitleID",
                              &TitleID::operator ==);
   RegisterFunctionExportName("__ne__Q3_2nn4boss7TitleIDCFRCQ3_2nn4boss7TitleID",
                              &TitleID::operator !=);
}

} // namespace cafe::nn::boss
