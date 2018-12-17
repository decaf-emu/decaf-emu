#pragma once

#define XXH_INLINE_ALL
#define XXH_CPU_LITTLE_ENDIAN 1
#define XXH_STATIC_LINKING_ONLY
#include "xxhash.h"
#include <array>
#include <functional>
#include <random>

class DataHash
{
public:
   inline DataHash()
   {
   }

   inline bool operator!=(const DataHash &rhs) const
   {
      return mHash != rhs.mHash;
   }

   inline bool operator==(const DataHash &rhs) const
   {
      return mHash == rhs.mHash;
   }

   inline DataHash& write(const void *data, size_t size)
   {
      mHash ^= XXH64(data, size, 0);
      return *this;
   }

   template <typename T>
   inline DataHash& write(const std::vector<T> &data)
   {
      static_assert(std::is_trivially_copyable<T>::value, "Hashed types must be trivial");
      static_assert(std::has_unique_object_representations<T>::value,
                    "Hashed types must have unique object representations");
      return write(data.data(), data.size() * sizeof(T));
   }

   template <typename T>
   inline DataHash& write(const T &data)
   {
      static_assert(std::is_trivially_copyable<T>::value, "Hashed types must be trivial");
      static_assert(std::has_unique_object_representations<T>::value,
                    "Hashed types must have unique object representations");
      return write(&data, sizeof(T));
   }

   inline uint64_t value() const
   {
      return mHash;
   }

   static inline DataHash random()
   {
      static std::random_device r;
      static std::mt19937 gen(r());
      static std::uniform_int_distribution<uint64_t> unidist;

      DataHash hash;
      hash.mHash = unidist(gen);
      return hash;
   }

private:
   uint64_t mHash = 0;

};

namespace std
{

template <> struct hash<DataHash>
{
   uint64_t operator()(const DataHash& x) const
   {
      return x.value();
   }
};

} // namespace std
