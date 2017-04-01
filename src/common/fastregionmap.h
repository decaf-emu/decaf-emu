#pragma once
#include <atomic>
#include <common/decaf_assert.h>
#include <cstring>
#include <vector>

static_assert(sizeof(std::atomic<void*>) == sizeof(void*), "This class assumes std::atomic has no overhead");

template<typename Type>
class FastRegionMap
{
public:
   FastRegionMap()
      : mData(nullptr)
   {
      clear();
   }

   ~FastRegionMap()
   {
      // Currently we do not clean up due to atomics
   }

   // Note that there must be no readers or writers to call this
   void clear()
   {
      if (mData) {
         for (auto i = 0u; i < 0x100; ++i) {
            auto level1 = mData[i].load();

            if (level1) {
               for (auto j = 0u; j < 0x100; ++j) {
                  auto level2 = level1[j].load();

                  if (level2) {
                     delete[] level2;
                  }
               }

               delete[] level1;
            }
         }
      } else {
         mData = new std::atomic<std::atomic<std::atomic<Type>*>*>[0x100];
      }

      memset(mData, 0, sizeof(void*) * 0x100);
   }

   std::atomic<Type> *getPtr(uint32_t location)
   {
      decaf_assert(!(location & 0x3), "Location was not a multiple of 4.");

      auto index1 = (location & 0xFF000000) >> 24;
      auto index2 = (location & 0x00FF0000) >> 16;
      auto index3 = (location & 0x0000FFFC) >> 2;

      auto level1 = mData[index1].load();

      if (UNLIKELY(!level1)) {
         auto newTable = new std::atomic<std::atomic<Type>*>[0x100];
         std::memset(newTable, 0, sizeof(void*) * 0x100);

         if (mData[index1].compare_exchange_strong(level1, newTable)) {
            level1 = newTable;
         } else {
            // compare_exchange updates level1 if we were preempted
            delete[] newTable;
         }
      }

      auto level2 = level1[index2].load();

      if (UNLIKELY(!level2)) {
         auto newTable = new std::atomic<Type>[0x4000];
         std::memset(newTable, 0, sizeof(Type) * 0x4000);

         if (level1[index2].compare_exchange_strong(level2, newTable)) {
            level2 = newTable;
         } else {
            // compare_exchange updates level2 if we were preempted
            delete[] newTable;
         }
      }

      return &level2[index3];
   }

   inline Type find(uint32_t location) const
   {
      auto index1 = (location & 0xFF000000) >> 24;
      auto index2 = (location & 0x00FF0000) >> 16;
      auto index3 = (location & 0x0000FFFC) >> 2;

      auto level1 = mData[index1].load();
      if (UNLIKELY(!level1)) {
         return 0;
      }

      auto level2 = level1[index2].load();
      if (UNLIKELY(!level2)) {
         return 0;
      }

      return level2[index3].load();
   }

   void set(uint32_t location, Type data)
   {
      auto ptr = getPtr(location);
      ptr->store(data);
   }

   bool getNextEntry(uint32_t *location_ptr, Type *value_ret) const
   {
      auto location = *location_ptr + 4;

      auto index1 = (location & 0xFF000000) >> 24;
      auto index2 = (location & 0x00FF0000) >> 16;
      auto index3 = (location & 0x0000FFFC) >> 2;

      for (; index1 < 0x100; index1++, index2 = 0) {
         auto level1 = mData[index1].load();
         if (level1) {
            for (; index2 < 0x100; index2++, index3 = 0) {
               auto level2 = level1[index2].load();
               if (level2) {
                  for (; index3 < 0x4000; index3++) {
                     auto value = level2[index3].load();
                     if (value) {
                        *location_ptr = index1<<24 | index2<<16 | index3<<2;
                        *value_ret = value;
                        return true;
                     }
                  }
               }
            }
         }
      }

      return false;
   }

   bool getFirstEntry(uint32_t *location_ret, Type *value_ret) const
   {
      *location_ret = 0xFFFFFFFC;
      return getNextEntry(location_ret, value_ret);
   }

private:
   std::atomic<std::atomic<std::atomic<Type>*>*>*
      mData;
};
