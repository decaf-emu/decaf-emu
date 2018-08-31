#pragma once
#include <atomic>
#include <common/byte_swap.h>
#include <common/type_traits.h>

template<typename Type>
class be2_atomic
{
public:
   static_assert(std::atomic<Type>::is_always_lock_free);
   static_assert(sizeof(std::atomic<Type>) == sizeof(Type));
   static_assert(sizeof(Type) == 1 || sizeof(Type) == 2 || sizeof(Type) == 4 || sizeof(Type) == 8);

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

   value_type
   fetch_add(value_type addValue)
   {
      auto oldValue = load(std::memory_order_relaxed);
      auto newValue = oldValue + addValue;

      while (!compare_exchange_weak(oldValue, newValue,
                                    std::memory_order_release,
                                    std::memory_order_relaxed)) {
         newValue = oldValue + addValue;
      }

      return oldValue;
   }

   value_type
   fetch_sub(value_type subValue)
   {
      auto oldValue = load(std::memory_order_relaxed);
      auto newValue = oldValue - subValue;

      while (!compare_exchange_weak(oldValue, newValue,
                                    std::memory_order_release,
                                    std::memory_order_relaxed)) {
         newValue = oldValue - subValue;
      }

      return oldValue;
   }

   value_type
   fetch_and(value_type subValue)
   {
      auto oldValue = load(std::memory_order_relaxed);
      auto newValue = oldValue & subValue;

      while (!compare_exchange_weak(oldValue, newValue,
                                    std::memory_order_release,
                                    std::memory_order_relaxed)) {
         newValue = oldValue & subValue;
      }

      return oldValue;
   }

   value_type
   fetch_or(value_type subValue)
   {
      auto oldValue = load(std::memory_order_relaxed);
      auto newValue = oldValue | subValue;

      while (!compare_exchange_weak(oldValue, newValue,
                                    std::memory_order_release,
                                    std::memory_order_relaxed)) {
         newValue = oldValue | subValue;
      }

      return oldValue;
   }

   value_type
   fetch_xor(value_type subValue)
   {
      auto oldValue = load(std::memory_order_relaxed);
      auto newValue = oldValue ^ subValue;

      while (!compare_exchange_weak(oldValue, newValue,
                                    std::memory_order_release,
                                    std::memory_order_relaxed)) {
         newValue = oldValue ^ subValue;
      }

      return oldValue;
   }

private:
   std::atomic<value_type> mStorage;
};
