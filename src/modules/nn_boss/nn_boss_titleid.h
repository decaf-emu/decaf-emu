#pragma once
#include "types.h"
#include "utils/be_val.h"
#include "utils/structsize.h"

/*
Unimplemented functions:
nn::boss::TitleID::operator==(nn::boss::TitleID const &) const
nn::boss::TitleID::operator!=(nn::boss::TitleID const &) const
*/

namespace nn
{

namespace boss
{

class TitleID
{
public:
   TitleID();
   TitleID(TitleID *other);
   TitleID(uint64_t id);

   bool
   IsValid();

   uint64_t
   GetValue();

   uint32_t
   GetTitleID();

   uint32_t
   GetTitleCode();

   uint32_t
   GetUniqueId();

private:
   be_val<uint32_t> mLower;
   be_val<uint32_t> mUpper;
};
CHECK_SIZE(TitleID, 8);

} // namespace boss

} // namespace nn
