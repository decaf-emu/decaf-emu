#pragma once
#include <string_view>

class ReplayParser
{
public:
   virtual ~ReplayParser() = default;

   virtual bool readFrame() = 0;
};
