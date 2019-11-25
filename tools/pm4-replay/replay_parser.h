#pragma once
#include <string_view>

class ReplayParser
{
public:
   virtual ~ReplayParser() = default;

   virtual bool runUntilTimestamp(uint64_t timestamp) = 0;
};
