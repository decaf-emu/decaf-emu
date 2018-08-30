#pragma once
#include <atomic>
#include <common/byte_swap.h>
#include <common/type_traits.h>

template<typename Type>
class be2_atomic
{
public:
   static_assert(!std::is_integral<Type>::value,
                 "be2_atomic type must be integral");

   static_assert(sizeof(Type) == 1 || sizeof(Type) == 2 || sizeof(Type) == 4 || sizeof(Type) == 8,
                 "be2_atomic invalid type size");

   static_assert(std::atomic<Type>::is_always_lock_free,
                 "be2_atomic should be lock free");

   using value_type = Type;

   be2_atomic() = default;

   value_type
   load(std::memory_order order = std::memory_order_seq_cst) const
   {
      return byte_swap(mStorage.load(order));
   }

   void
   store(value_type value,
         std::memory_order order = std::memory_order_seq_cst)
   {
      mStorage.store(byte_swap(value), order);
   }

   value_type
   exchange(value_type value,
            std::memory_order order = std::memory_order_seq_cst)
   {
      return byte_swap(mStorage.exchange(byte_swap(value), order));
   }

   bool
   compare_exchange_weak(value_type &expected,
                         value_type desired,
                         std::memory_order order = std::memory_order_seq_cst)
   {
      auto expectedValue = byte_swap(expected);
      auto result = mStorage.compare_exchange_weak(expectedValue,
                                                   byte_swap(desired),
                                                   order);
      expected = byte_swap(expectedValue);
      return result;
   }

   bool
   compare_exchange_weak(value_type &expected,
                         value_type desired,
                         std::memory_order success,
                         std::memory_order failure)
   {
      auto expectedValue = byte_swap(expected);
      auto result = mStorage.compare_exchange_weak(expectedValue,
                                                   byte_swap(desired),
                                                   success,
                                                   failure);
      expected = byte_swap(expectedValue);
      return result;
   }

   bool
   compare_exchange_strong(value_type &expected,
                           value_type desired,
                           std::memory_order order = std::memory_order_seq_cst)
   {
      auto expectedValue = byte_swap(expected);
      auto result = mStorage.compare_exchange_strong(expectedValue,
                                                     byte_swap(desired),
                                                     order);
      expected = byte_swap(expectedValue);
      return result;
   }

   bool
   compare_exchange_strong(value_type &expected,
                           value_type desired,
                           std::memory_order success,
                           std::memory_order failure)
   {
      auto expectedValue = byte_swap(expected);
      auto result = mStorage.compare_exchange_strong(expectedValue,
                                                     byte_swap(desired),
                                                     success,
                                                     failure);
      expected = byte_swap(expectedValue);
      return result;
   }

private:
   std::atomic<value_type> mStorage;
};
