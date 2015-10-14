#pragma once
#include <vector>
#include <map>

template<typename PpcType>
struct HostLookupItem
{
   PpcType *source = nullptr;
};

template<typename HostClass, typename PpcClass>
class HostLookupTable {
   static_assert(std::is_base_of<HostLookupItem<PpcClass>, HostClass>::value, "HostLookup items must inherit from HostLookupItem");

protected:
   struct Data
   {
      PpcClass *source = nullptr;
      HostClass item;
   };

public:
   HostClass *
   get(PpcClass *source)
   {
      auto itr = mData.find(source);

      if (itr != mData.end()) {
         return itr->second;
      }

      HostClass *data = new HostClass();
      data->source = source;
      data->alloc();
      mData.emplace(source, data);
      return data;
   }

   void freeRange(void *ptr, ppcsize_t n)
   {
   }

protected:
   std::map<PpcClass *, HostClass *> mData;
};
