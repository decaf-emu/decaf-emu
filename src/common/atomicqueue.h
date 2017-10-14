#pragma once
#include <atomic>

/**
 * Multi-producer multi-consumer queue.
 * This is unsafe and full of assumptions, should be very careful about using it.
 *
 * This is not a safe queue, it does not check if the queue is full before
 * pushing, or if it is empty before popping.
 *
 * Size must be a power of two so you don't need to be careful for wrapping of
 * read / write position.
 *
 * Empty when writePos = readPos.
 * Full when writePos + 1 == readPos.
 */
template<typename Type, std::size_t Size>
class alignas(64) AtomicQueue
{
   static_assert(Size && ((Size & (Size - 1)) == 0), "N must be a power of two");

public:
   constexpr std::size_t capacity() const
   {
      return N - 1;
   }

   bool wasFull() const
   {
      return (mWritePosition.load() + 1) == mReadPosition.load();
   }

   bool wasEmpty() const
   {
      return mWritePosition.load() == mReadPosition.load();
   }

   void push(Type value)
   {
      auto writePos = mWritePosition.fetch_add(1);
      mBuffer[writePos % Size] = value;
   }

   Type pop()
   {
      auto readPos = mReadPosition.fetch_add(1);
      return mBuffer[readPos % Size];
   }

private:
   alignas(64) std::atomic<std::size_t> mWritePosition = 0;
   Type mBuffer[Size];
   alignas(64) std::atomic<std::size_t> mReadPosition = 0;
};

/**
 * Single producer, single consumer queue.
 * Much safer than MultiAtomicQueue, preferred when only one thread is writing
 * and one thread is reading.
 *
 * Safe, can detect whether queue is full on push, or empty on pop.
 *
 * Does not require size to be power of two.
 *
 * Empty when read == write
 * Full when (write + 1) == read
 */
template<typename Type, std::size_t Size>
class SingleAtomicQueue
{
public:
   constexpr std::size_t capacity() const
   {
      return Size - 1;
   }

   bool push(Type value)
   {
      const auto writePos = mWritePosition.load(std::memory_order_relaxed);
      const auto nextWritePos = (writePos + 1) % Size;

      if (nextWritePos == mReadPosition.load(std::memory_order_acquire)) {
         // Queue is full!
         return false;
      }

      mBuffer[writePos] = value;
      mWritePosition.store(writePos, std::memory_order_release);
      return true;
   }

   bool pop(Type &value)
   {
      const auto readPos = mReadPosition.load(std::memory_order_relaxed);
      if (readPos == mWritePosition.load(std::memory_order_acquire)) {
         // Queue is empty!
         return false;
      }

      value = mBuffer[readPos];
      mReadPosition.store((readPos + 1) % Size, std::memory_order_release);
      return true;
   }

private:
   alignas(64) std::atomic<std::size_t> mWritePosition = 0;
   Type mBuffer[Size];
   alignas(64) std::atomic<std::size_t> mReadPosition = 0;
};
