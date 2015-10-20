#pragma once
#include <cstdint>
#include <vector>

static const std::vector<uint32_t> gValuesCRB =
{
   0,
   1,
};

static const std::vector<uint32_t> gValuesGPR =
{
   0,
   1,
   static_cast<uint32_t>(-1),
   static_cast<uint32_t>(std::numeric_limits<int32_t>::min()),
   static_cast<uint32_t>(std::numeric_limits<int32_t>::max()),
   53, 0x12345678, 0x87654321
};

static const std::vector<int16_t> gValuesSIMM =
{
   0,
   1,
   -1,
   std::numeric_limits<int16_t>::min(),
   std::numeric_limits<int16_t>::max(),
   53, 0x1234, static_cast<int16_t>(0x8765u)
};

static const std::vector<uint16_t> gValuesUIMM =
{
   0,
   1,
   static_cast<uint16_t>(-1),
   static_cast<uint16_t>(std::numeric_limits<int16_t>::min()),
   static_cast<uint16_t>(std::numeric_limits<int16_t>::max()),
   53, 0x1234, 0x8765
};

static const std::vector<double> gValuesFPR =
{
   0.0,
   -0.0,
   1.0,
   -1.0,
   std::numeric_limits<double>::min(),
   std::numeric_limits<double>::max(),
   std::numeric_limits<double>::lowest(),
   std::numeric_limits<double>::infinity(),
   -std::numeric_limits<double>::infinity(),
   std::numeric_limits<double>::quiet_NaN(),
   std::numeric_limits<double>::signaling_NaN(),
   std::numeric_limits<double>::denorm_min(),
   std::numeric_limits<double>::epsilon(),
   33525.78
};

static const std::vector<uint32_t> gValuesXERC =
{
   0,
   1
};

static const std::vector<uint32_t> gValuesXERSO =
{
   0,
   1
};

static const std::vector<uint32_t> gValuesSH =
{
   0, 15, 23, 31
};

static const std::vector<uint32_t> gValuesMB =
{
   0, 15, 23, 31
};

static const std::vector<uint32_t> gValuesME =
{
   0, 15, 23, 31
};
