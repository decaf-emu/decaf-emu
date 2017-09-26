#pragma once
#include "ios_enum.h"

#include <array>
#include <memory>
#include <type_traits>

namespace ios
{

template<typename ValueType, typename HandleType, size_t MaxNumHandles>
class HandleManager
{
   static_assert(MaxNumHandles < 0xFFFF);

   struct Handle
   {
      uint16_t instanceNum = 0;
      std::unique_ptr<ValueType> value = nullptr;
   };

public:
   Error
   open()
   {
      auto index = -1;

      for (auto i = 0u; i < mHandles.size(); ++i) {
         if (!mHandles[i].value) {
            index = static_cast<int>(i);
            break;
         }
      }

      if (index < 0) {
         return Error::Max;
      }

      ++mHandles[index].instanceNum;
      mHandles[index].value = std::make_unique<ValueType>();
      return static_cast<Error>((mHandles[index].instanceNum << 16) | index);
   }

   Error
   close(HandleType handle)
   {
      auto index = handle & 0xFFFF;
      auto instanceNum = (handle >> 16) & 0xFFFF;

      if constexpr (std::is_signed<HandleType>::value) {
         if (index < 0) {
            return Error::InvalidHandle;
         }
      }

      if (index >= mHandles.size()) {
         return Error::InvalidHandle;
      }

      if (!mHandles[index].value || mHandles[index].instanceNum != instanceNum) {
         return Error::StaleHandle;
      }

      mHandles[index].value = nullptr;
      return Error::OK;
   }

   Error
   close(ValueType *value)
   {
      for (auto i = 0u; i < mHandles.size(); ++i) {
         if (mHandles[i].value.get() == value) {
            mHandles[i].value = nullptr;
            return Error::OK;
         }
      }

      return Error::InvalidHandle;
   }

   Error
   get(HandleType handle,
       ValueType **outData)
   {
      auto index = handle & 0xFFFF;
      auto instanceNum = (handle >> 16) & 0xFFFF;

      if constexpr (std::is_signed<HandleType>::value) {
         if (index < 0) {
            return Error::InvalidHandle;
         }
      }

      if (index >= mHandles.size()) {
         return Error::InvalidHandle;
      }

      if (!mHandles[index].value || mHandles[index].instanceNum != instanceNum) {
         return Error::StaleHandle;
      }

      *outData = mHandles[index].value.get();
      return Error::OK;
   }

private:
   std::array<Handle, MaxNumHandles> mHandles;
};

} // namespace ios
