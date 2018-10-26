#pragma once

#include "murmur3.h"
#include <array>
#include <functional>
#include <random>

class DataHash
{
public:
   using value_type = std::array<uint64_t, 2>;

   inline DataHash()
   {
   }

   inline bool operator!=(const DataHash &rhs) const
   {
      return mHash[0] != rhs.mHash[0] || mHash[1] != rhs.mHash[1];
   }

   inline bool operator==(const DataHash &rhs) const
   {
      return mHash[0] == rhs.mHash[0] && mHash[1] == rhs.mHash[1];
   }

   inline DataHash& write(const void *data, size_t size)
   {
      uint64_t newData[2];
      MurmurHash3_x64_128(data, static_cast<int>(size), 0, &newData);
      mHash[0] ^= newData[0];
      mHash[1] ^= newData[1];
      return *this;
   }

   template <typename T>
   inline DataHash& write(const std::vector<T> &data)
   {
      static_assert(std::is_trivially_copyable<T>::value, "Hashed types must be trivial");
      return write(data.data(), data.size() * sizeof(T));
   }

   template <typename T>
   inline DataHash& write(const T &data)
   {
      static_assert(std::is_trivially_copyable<T>::value, "Hashed types must be trivial");
      return write(&data, sizeof(T));
   }

   inline value_type value() const
   {
      return mHash;
   }

   inline uint64_t fastCompareValue() const
   {
      return mHash[0] ^ mHash[1];
   }

   static inline DataHash random()
   {
      static std::random_device r;
      static std::mt19937 gen(r());
      static std::uniform_int_distribution<uint64_t> unidist;

      DataHash hash;
      hash.mHash[0] = unidist(gen);
      hash.mHash[1] = unidist(gen);
      return hash;
   }

private:
   value_type mHash = { 0 };

};

namespace std
{

template <> struct hash<DataHash>
{
   uint64_t operator()(const DataHash& x) const
   {
      return x.fastCompareValue();
   }
};

} // namespace std
