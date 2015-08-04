#pragma once
#include <vector>

template<typename HostClass, typename PpcClass>
class HostLookupTable {
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

      auto& data = mArray[idx];
      if (data.source != source) {
         return alloc(source);
      }

      return &data.item;
   }

protected:
   void free(uint32_t index) {
      mArray[index].source = nullptr;
   }

   HostClass * alloc(PpcClass *source) {
      if (mAvailable == 0) {
         assert(mArray.size() < 0xFFFFFFFF);

         source->driverData._index = (uint32_t)mArray.size();
         mArray.emplace_back();
         auto& data = mArray.back();
         return &data.item;
      }

      for (uint32_t idx = 0; idx < mArray.size(); ++idx) {
         auto &i = mArray[idx];
         if (i.source == nullptr) {
            source->driverData._index = idx;
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
