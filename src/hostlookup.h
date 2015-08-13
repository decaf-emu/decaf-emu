#pragma once
#include <vector>

template<typename PpcType>
struct HostLookupItem {
   PpcType *source;

};

template<typename HostClass, typename PpcClass>
class HostLookupTable {
   static_assert(std::is_base_of<HostLookupItem<PpcClass>, HostClass>::value, "HostLookup items must inherit from HostLookupItem");
   static_assert(&PpcClass::driverData, "Lookup types must have driverData field");

protected:
   struct Data {
      PpcClass *source;
      HostClass item;
   };

public:
   HostLookupTable() {
      mAvailable = 0;
      mArray.reserve(100);
   }

   HostClass * get(PpcClass *source) {
      auto idx = source->driverData._index;

      if (idx == 0) {
         return alloc(source);
      }

      auto& data = mArray[idx - 1];
      if (data.source != source) {
         return alloc(source);
      }

      return &data.item;
   }

   void freeRange(void *ptr, ppcsize_t n) {
      for (auto i = 0; i < mArray.size(); ++i) {
         auto& item = mArray[i];
         if ((uint8_t*)item.source >= (uint8_t*)ptr &&
            (uint8_t*)item.source < (uint8_t*)ptr + n) {
            free(i);
         }
      }
   }

protected:
   void free(uint32_t index) {
      auto& data = mArray[index - 1];
      data.item.release();
      data.item.source = nullptr;
      data.source = nullptr;
   }

   HostClass * alloc(PpcClass *source) {
      if (mAvailable == 0) {
         assert(mArray.size() < 0xFFFFFFFF);

         source->driverData._index = (uint32_t)mArray.size() + 1;
         mArray.emplace_back();
         auto& data = mArray.back();
         data.source = source;
         data.item.source = source;
         data.item.alloc();
         return &data.item;
      }

      for (uint32_t idx = 0; idx < mArray.size(); ++idx) {
         auto &i = mArray[idx];
         if (i.source == nullptr) {
            source->driverData._index = idx + 1;
            i.source = source;
            i.item.source = source;
            i.item.alloc();
            return &i.item;
         }
      }

      // if mAvailable > 0, we should never reach here
      assert(0);
      return 0;
   }


   uint64_t mAvailable;
   std::vector<Data> mArray;

};
