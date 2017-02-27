#pragma once
#include <atomic>
#include <common/decaf_assert.h>

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

   Type find(uint32_t location)
   {
      auto index1 = (location & 0xFF000000) >> 24;
      auto index2 = (location & 0x00FF0000) >> 16;
      auto index3 = (location & 0x0000FFFC) >> 2;
      auto level1 = mData[index1].load();

      if (!level1) {
         return nullptr;
      }

      auto level2 = level1[index2].load();

      if (!level2) {
         return nullptr;
      }

      return level2[index3].load();
   }

   void set(uint32_t location, Type data)
   {
      if (location & 0x3) {
         decaf_abort("Location was not power-of-two.");
      }

      auto index1 = (location & 0xFF000000) >> 24;
      auto index2 = (location & 0x00FF0000) >> 16;
      auto index3 = (location & 0x0000FFFC) >> 2;
      auto level1 = mData[index1].load();

      if (!level1) {
         auto newTable = new std::atomic<std::atomic<Type>*>[0x100];
         std::memset(newTable, 0, sizeof(void*) * 0x100);

         if (mData[index1].compare_exchange_strong(level1, newTable)) {
            level1 = newTable;
         } else {
            // compare_exchange updates level1 if we were pre-empted
            delete[] newTable;
         }
      }

      auto level2 = level1[index2].load();

      if (!level2) {
         auto newTable = new std::atomic<Type>[0x4000];
         std::memset(newTable, 0, sizeof(void*) * 0x4000);

         if (level1[index2].compare_exchange_strong(level2, newTable)) {
            level2 = newTable;
         } else {
            // compare_exchange updates level2 if we were preempted
            delete[] newTable;
         }
      }

      level2[index3].store(data);
   }

private:
   std::atomic<std::atomic<std::atomic<Type>*>*>*
      mData;

};