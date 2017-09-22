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
template<typename Type, size_t N>
class alignas(64) AtomicQueue
{
   static_assert(N && ((N & (N - 1)) == 0), "N must be a power of two");

public:
   constexpr size_t capacity() const
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
      mBuffer[writePos % N] = value;
   }

   Type pop()
   {
      auto readPos = mReadPosition.fetch_add(1);
      return mBuffer[readPos % N];
   }

private:
   alignas(64) std::atomic<size_t> mWritePosition = 0;
   Type mBuffer[N];
   alignas(64) std::atomic<size_t> mReadPosition = 0;
};
