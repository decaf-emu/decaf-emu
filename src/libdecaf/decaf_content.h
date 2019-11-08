#pragma once
#include <cstdint>

namespace decaf
{

enum class TitleType
{
   Application    = 0,
   Demo           = 2,
   Data           = 0xB,
   DLC            = 0xC,
   Update         = 0xE,
};

using TitleID = uint64_t;

constexpr inline bool isSystemTitle(TitleID id)
{
   return !!((id >> 32) & 0x10);
}

constexpr inline TitleType getTitleTypeFromID(TitleID id)
{
   return static_cast<TitleType>((id >> 32) & 0x0f);
}

} // namespace decaf
