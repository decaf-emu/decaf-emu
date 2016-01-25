#pragma once

namespace coreinit
{

template<typename QueueType>
static void inline
OSInitQueue(QueueType *queue)
{
   queue->head = nullptr;
   queue->tail = nullptr;
}

template<typename QueueType>
static void inline
OSInitQueueEx(QueueType *queue, void *parent)
{
   OSInitQueue(queue);
   queue->parent = parent;
}

template<typename LinkType>
static void inline
OSInitQueueLink(LinkType *link)
{
   link->prev = nullptr;
   link->next = nullptr;
}

// Returns true if queue is empty
template<typename QueueType>
static bool inline
OSIsEmptyQueue(QueueType *queue)
{
   return queue->head == nullptr;
}

// Clear queue
template<typename QueueType>
static void inline
OSClearQueue(QueueType *queue)
{
   for (auto item = queue->head; item; ) {
      auto next = item->link.next;
      item->queue = nullptr;
      item->link.next = nullptr;
      item->link.prev = nullptr;
      item = next;
   }

   queue->head = nullptr;
   queue->tail = nullptr;
}

// Append item to back of queue
template<typename QueueType, typename ItemType>
static void inline
OSAppendQueue(QueueType *queue, ItemType *item)
{
   if (!queue->tail) {
      queue->head = item;
      queue->tail = item;
      item->link.next = nullptr;
      item->link.prev = nullptr;
   } else {
      item->link.prev = queue->tail;
      item->link.next = nullptr;
      queue->tail->link.next = item;
      queue->tail = item;
   }
}

// Erase item from the queue
template<typename QueueType, typename ItemType>
static void inline
OSEraseFromQueue(QueueType *queue, ItemType *item)
{
   if (queue->head == item) {
      // Erase from head
      queue->head = item->link.next;

      if (queue->head) {
         queue->head->link.prev = nullptr;
      } else {
         queue->tail = nullptr;
      }
   } else if (queue->tail == item) {
      // Erase from tail
      queue->tail = item->link.prev;

      if (queue->tail) {
         queue->tail->link.next = nullptr;
      }
   } else {
      // Erase from middle
      auto prev = item->link.prev;
      auto next = item->link.next;
      prev->link.next = next;
      next->link.prev = prev;
   }
}

// Remove and return the item at front of the queue
template<typename QueueType>
static auto inline
OSPopFrontFromQueue(QueueType *queue)
{
   auto result = queue->head;

   if (result) {
      queue->head = result->link.next;

      if (queue->head) {
         queue->head->link.prev = nullptr;
      }
   }

   if (result == queue->tail) {
      queue->tail = nullptr;
   }

   return result;
}

} // namespace coreinit
