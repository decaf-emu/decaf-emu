#pragma once
#include <common/decaf_assert.h>
#include <libcpu/be2_struct.h>

namespace cafe::coreinit::internal
{

template <typename QueueType, typename LinkType, typename ItemType, be2_struct<LinkType> ItemType::*LinkField>
class Queue
{
protected:
   static constexpr LinkType &
   link(virt_ptr<ItemType> item)
   {
      return (item.getRawPointer()->*LinkField);
   }

public:
   static inline void
   init(virt_ptr<QueueType> queue)
   {
      queue->head = nullptr;
      queue->tail = nullptr;
   }

   static inline void
   initLink(virt_ptr<ItemType> item)
   {
      link(item).prev = nullptr;
      link(item).next = nullptr;
   }

   static inline bool
   empty(virt_ptr<QueueType> queue)
   {
      return queue->head == nullptr;
   }

   static inline void
   clear(virt_ptr<QueueType> queue)
   {
      for (auto item = queue->head; item; ) {
         auto next = link(item).next;
         link(item).next = nullptr;
         link(item).prev = nullptr;
         item = next;
      }

      queue->head = nullptr;
      queue->tail = nullptr;
   }

   static inline bool
   contains(virt_ptr<QueueType> queue,
            virt_ptr<ItemType> item)
   {
      for (auto itemIter = queue->head; itemIter != nullptr; itemIter = link(itemIter).next) {
         if (itemIter == item) {
            return true;
         }
      }

      return false;
   }

   static inline void
   append(virt_ptr<QueueType> queue,
          virt_ptr<ItemType> item)
   {
      decaf_check(link(item).next == nullptr);
      decaf_check(link(item).prev == nullptr);

      if (!queue->tail) {
         decaf_check(!queue->head);

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
   erase(virt_ptr<QueueType> queue,
         virt_ptr<ItemType> item)
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

      link(item).next = nullptr;
      link(item).prev = nullptr;
   }

   static inline virt_ptr<ItemType>
   popFront(virt_ptr<QueueType> queue)
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

      if (result) {
         link(result).next = nullptr;
         link(result).prev = nullptr;
      }

      return result;
   }
};

template <typename QueueType, typename LinkType, typename ItemType, be2_struct<LinkType> ItemType::*LinkField, typename IsLess>
class SortedQueue : public Queue<QueueType, LinkType, ItemType, LinkField>
{
private:
   // Hide append as it is not valid here
   using Queue<QueueType, LinkType, ItemType, LinkField>::append;
   using Queue<QueueType, LinkType, ItemType, LinkField>::link;

public:
   static void inline
   insert(virt_ptr<QueueType> queue,
          virt_ptr<ItemType> item)
   {
      decaf_check(link(item).next == nullptr);
      decaf_check(link(item).prev == nullptr);

      if (!queue->head) {
         // Insert only item
         queue->head = item;
         queue->tail = item;
      } else {
         virt_ptr<ItemType> insertBefore = nullptr;

         // Find insert location based on sort function
         for (insertBefore = queue->head; insertBefore; insertBefore = link(insertBefore).next) {
            if (!IsLess {}(insertBefore, item)) {
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
            // Insert in head or middle
            link(item).next = insertBefore;
            link(item).prev = link(insertBefore).prev;

            if (link(insertBefore).prev) {
               link(link(insertBefore).prev).next = item;
            }

            link(insertBefore).prev = item;

            if (queue->head == insertBefore) {
               queue->head = item;
            }
         }
      }
   }
};

} // namespace cafe::coreinit::internal
