#pragma once

#include <catch.hpp>
#include <fmt/format.h>
#include <random>
#include <vector>

static inline std::vector<uint8_t>
generateRandomData(size_t size)
{
   std::mt19937 eng { 0x0DECAF10 };
   std::uniform_int_distribution<uint32_t> urd { 0, 255 };
   std::vector<uint8_t> result;
   result.resize(size);

   for (auto i = 0; i < size; ++i) {
      result[i] = static_cast<uint8_t>(urd(eng));
   }

   return result;
}

static inline bool
compareImages(const std::vector<uint8_t> &data,
              const std::vector<uint8_t> &reference)
{
   REQUIRE(data.size() == reference.size());

   for (auto i = 0u; i < data.size(); ++i) {
      if (data[i] != reference[i]) {
         WARN(fmt::format("Difference at offset {}, 0x{:02X} != 0x{:02X}", i, data[i], reference[i]));
         return false;
      }
   }

   return true;
}
