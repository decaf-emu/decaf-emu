#pragma once

namespace coreinit
{

namespace internal
{

template <typename QueueType, typename LinkType, typename ItemType, LinkType ItemType::*LinkField>
class QueueFuncs {
public:
   static void inline
      init(QueueType *queue)
   {
      queue->head = nullptr;
      queue->tail = nullptr;
   }

   static void inline
      initItemLink(ItemType *item)
   {
      (item->*LinkField).prev = nullptr;
      (item->*LinkField).next = nullptr;
   }

   static bool inline
      isEmpty(QueueType *queue)
   {
      return queue->head == nullptr;
   }

   static void inline
      clear(QueueType *queue)
   {
      for (auto item = queue->head; item; ) {
         auto next = (item->*LinkField).next;
         item->queue = nullptr;
         (item->*LinkField).next = nullptr;
         (item->*LinkField).prev = nullptr;
         item = next;
      }

      queue->head = nullptr;
      queue->tail = nullptr;
   }

   static void inline
      append(QueueType *queue, ItemType *item)
   {
      if (!queue->tail) {
         queue->head = item;
         queue->tail = item;
         (item->*LinkField).next = nullptr;
         (item->*LinkField).prev = nullptr;
      } else {
         (item->*LinkField).prev = queue->tail;
         (item->*LinkField).next = nullptr;
         (queue->tail->*LinkField).next = item;
         queue->tail = item;
      }
   }

   static void inline
      erase(QueueType *queue, ItemType *item)
   {
      if (queue->head == item) {
         // Erase from head
         queue->head = (item->*LinkField).next;

         if (queue->head) {
            (queue->head->*LinkField).prev = nullptr;
         } else {
            queue->tail = nullptr;
         }
      } else if (queue->tail == item) {
         // Erase from tail
         queue->tail = (item->*LinkField).prev;

         if (queue->tail) {
            (queue->tail->*LinkField).next = nullptr;
         }
      } else {
         // Erase from middle
         auto prev = (item->*LinkField).prev;
         auto next = (item->*LinkField).next;
         (prev->*LinkField).next = next;
         (next->*LinkField).prev = prev;
      }
   }

   static inline ItemType *
      popFront(QueueType *queue)
   {
      auto result = queue->head;

      if (result) {
         queue->head = (result->*LinkField).next;

         if (queue->head) {
            (queue->head->*LinkField).prev = nullptr;
         }
      }

      if (result == queue->tail) {
         queue->tail = nullptr;
      }

      return result;
   }

};

template <typename QueueType, typename LinkType, typename ItemType, LinkType ItemType::*LinkField, bool SortFunc(ItemType*,ItemType*)>
class SortedQueueFuncs {
   using MyQueueFuncs = QueueFuncs<QueueType, LinkType, ItemType, LinkField>;

public:
   static void inline
      init(QueueType *queue)
   {
      MyQueueFuncs::init(queue);
   }

   static void inline
      initItemLink(ItemType *item)
   {
      MyQueueFuncs::initItemLink(item);
   }

   static bool inline
      isEmpty(QueueType *queue)
   {
      return MyQueueFuncs::isEmpty(queue);
   }

   static void inline
      clear(QueueType *queue)
   {
      MyQueueFuncs::clear(queue);
   }

   static void inline
      insert(QueueType *queue, ItemType *item)
   {
      if (!queue->head) {
         // Insert only item
         (item->*LinkField).prev = nullptr;
         (item->*LinkField).next = nullptr;
         queue->head = item;
         queue->tail = item;
      } else {
         OSThread *insertBefore = nullptr;

         // Find insert location based on priority
         for (insertBefore = queue->head; insertBefore; insertBefore = (insertBefore->*LinkField).next) {
            if (SortFunc(insertBefore, item)) {
               break;
            }
         }

         if (!insertBefore) {
            // Insert at tail
            (queue->tail->*LinkField).next = item;
            (item->*LinkField).next = nullptr;
            (item->*LinkField).prev = queue->tail;
            queue->tail = item;
         } else {
            // Insert in middle
            (item->*LinkField).next = insertBefore;
            (item->*LinkField).prev = (insertBefore->*LinkField).prev;

            if ((insertBefore->*LinkField).prev) {
               ((insertBefore->*LinkField).prev->*LinkField).next = item;
            }

            (insertBefore->*LinkField).prev = item;
         }
      }
   }

   static void inline
      erase(QueueType *queue, ItemType *item)
   {
      MyQueueFuncs::erase(queue, item);
   }

   static inline ItemType *
      popFront(QueueType *queue)
   {
      return MyQueueFuncs::popFront(queue);
   }

};

} // namespace internal

} // namespace coreinit
