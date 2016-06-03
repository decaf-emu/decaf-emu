#pragma once
#include "utils/emuassert.h"

namespace coreinit
{

namespace internal
{

template <typename QueueType, typename LinkType, typename ItemType, LinkType ItemType::*LinkField>
class Queue
{
protected:
   static constexpr LinkType &
   link(ItemType *item)
   {
      return (item->*LinkField);
   }

public:
   static inline void
   init(QueueType *queue)
   {
      queue->head = nullptr;
      queue->tail = nullptr;
   }

   static inline void
   initLink(ItemType *item)
   {
      link(item).prev = nullptr;
      link(item).next = nullptr;
   }

   static inline bool
   empty(QueueType *queue)
   {
      return queue->head == nullptr;
   }

   static inline void
   clear(QueueType *queue)
   {
      for (auto item = queue->head; item; ) {
         auto next = link(item).next;
         item->queue = nullptr;
         link(item).next = nullptr;
         link(item).prev = nullptr;
         item = next;
      }

      queue->head = nullptr;
      queue->tail = nullptr;
   }

   static inline void
   append(QueueType *queue, ItemType *item)
   {
      emuassert(link(item).next == nullptr);
      emuassert(link(item).prev == nullptr);

      if (!queue->tail) {
         queue->head = item;
         queue->tail = item;
         link(item).next = nullptr;
         link(item).prev = nullptr;
      } else {
         link(item).prev = queue->tail;
         link(item).next = nullptr;
         link(queue->tail).next = item;
         queue->tail = item;
      }
   }

   static inline void
   erase(QueueType *queue, ItemType *item)
   {
      if (queue->head == item) {
         // Erase from head
         queue->head = link(item).next;

         if (queue->head) {
            link(queue->head).prev = nullptr;
         } else {
            queue->tail = nullptr;
         }
      } else if (queue->tail == item) {
         // Erase from tail
         queue->tail = link(item).prev;

         if (queue->tail) {
            link(queue->tail).next = nullptr;
         }
      } else {
         // Erase from middle
         auto prev = link(item).prev;
         auto next = link(item).next;

         if (prev && next) {
            link(prev).next = next;
            link(next).prev = prev;
         }
      }
   }

   static inline ItemType *
   popFront(QueueType *queue)
   {
      auto result = queue->head;

      if (result) {
         queue->head = link(result).next;

         if (queue->head) {
            link(queue->head).prev = nullptr;
         }
      }

      if (result == queue->tail) {
         queue->tail = nullptr;
      }

      return result;
   }
};

template <typename QueueType, typename LinkType, typename ItemType, LinkType ItemType::*LinkField, bool SortFunc(ItemType*,ItemType*)>
class SortedQueue : public Queue<QueueType, LinkType, ItemType, LinkField>
{
public:
   static void inline
   insert(QueueType *queue, ItemType *item)
   {
      if (!queue->head) {
         // Insert only item
         link(item).prev = nullptr;
         link(item).next = nullptr;
         queue->head = item;
         queue->tail = item;
      } else {
         OSThread *insertBefore = nullptr;

         // Find insert location based on priority
         for (insertBefore = queue->head; insertBefore; insertBefore = link(insertBefore).next) {
            if (SortFunc(insertBefore, item)) {
               break;
            }
         }

         if (!insertBefore) {
            // Insert at tail
            link(queue->tail).next = item;
            link(item).next = nullptr;
            link(item).prev = queue->tail;
            queue->tail = item;
         } else {
            // Insert in middle
            link(item).next = insertBefore;
            link(item).prev = link(insertBefore).prev;

            if (link(insertBefore).prev) {
               link(link(insertBefore).prev).next = item;
            }

            link(insertBefore).prev = item;
         }
      }
   }
};

} // namespace internal

} // namespace coreinit
