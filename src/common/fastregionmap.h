#pragma once
#include <atomic>
#include <stdexcept>

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
      uint32_t level1Index = (location & 0xFF000000) >> 24;
      auto level1Data = mData[level1Index].load();
      if (!level1Data) {
         return nullptr;
      }

      uint32_t level2Index = (location & 0x00FF0000) >> 16;
      auto level2Data = level1Data[level2Index].load();
      if (!level2Data) {
         return nullptr;
      }

      uint32_t level3Index = (location & 0x0000FFFC) >> 2;
      return level2Data[level3Index].load();
   }

   void set(uint32_t location, Type data)
   {
      if (location & 0x3) {
         throw std::logic_error("Location was not power-of-two.");
      }

      uint32_t level1Index = (location & 0xFF000000) >> 24;
      auto level1Data = mData[level1Index].load();
      if (!level1Data) {
         auto newTable = new std::atomic<std::atomic<Type>*>[0x100];
         memset(newTable, 0, sizeof(void*) * 0x100);
         if (mData[level1Index].compare_exchange_strong(level1Data, newTable)) {
            level1Data = newTable;
         } else {
            // compare_exchange updates level1Data if we were preempted
            delete[] newTable;
         }
      }

      uint32_t level2Index = (location & 0x00FF0000) >> 16;
      auto level2Data = level1Data[level2Index].load();
      if (!level2Data) {
         auto newTable = new std::atomic<Type>[0x4000];
         memset(newTable, 0, sizeof(void*) * 0x4000);

         if (level1Data[level2Index].compare_exchange_strong(level2Data, newTable)) {
            level2Data = newTable;
         } else {
            // compare_exchange updates level2Data if we were preempted
            delete[] newTable;
         }
      }

      uint32_t level3Index = (location & 0x0000FFFC) >> 2;
      level2Data[level3Index].store(data);
   }

private:
   std::atomic<std::atomic<std::atomic<Type>*>*>*
      mData;

};