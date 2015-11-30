#include "coreinit.h"
#include "coreinit_memlist.h"

static MemoryLink *
getLink(MemoryList *list, void *object)
{
   return reinterpret_cast<MemoryLink*>(reinterpret_cast<uint8_t*>(object) + list->offsetToMemoryLink);
}

void
setFirstObject(MemoryList *list, void *object)
{
   auto link = getLink(list, object);
   list->head = object;
   list->tail = object;
   link->next = nullptr;
   link->prev = nullptr;
   list->count = 1;
}

void
MEMInitList(MemoryList *list, uint16_t offsetToMemoryLink)
{
   list->head = nullptr;
   list->tail = nullptr;
   list->count = 0;
   list->offsetToMemoryLink = offsetToMemoryLink;
}

void
MEMAppendListObject(MemoryList *list, void *object)
{
   if (!list->tail) {
      setFirstObject(list, object);
      return;
   }

   auto link = getLink(list, object);
   auto tail = getLink(list, list->tail);
   tail->next = object;
   link->prev = list->tail;
   link->next = nullptr;
   list->count++;
}

void
MEMPrependListObject(MemoryList *list, void *object)
{
   if (!list->head) {
      setFirstObject(list, object);
      return;
   }

   auto link = getLink(list, object);
   auto head = getLink(list, list->head);
   head->prev = object;
   link->prev = nullptr;
   link->next = list->head;
   list->head = object;
   list->count++;
}

void
MEMInsertListObject(MemoryList *list, void *before, void *object)
{
   if (!before) {
      // Insert at end
      MEMAppendListObject(list, object);
      return;
   }

   if (list->head == before) {
      // Insert before head
      MEMPrependListObject(list, object);
      return;
   }

   // Insert to middle of list
   auto link = getLink(list, object);
   auto other = getLink(list, before);
   link->prev = other->prev;
   link->next = before;
   other->prev = object;
   list->count++;
}

void
MEMRemoveListObject(MemoryList *list, void *object)
{
   void *head = nullptr;

   if (!object) {
      return;
   }

   if (list->head == object && list->tail == object) {
      // Clear list
      list->head = nullptr;
      list->tail = nullptr;
      list->count = 0;
      return;
   }

   if (list->head == object) {
      // Remove from head
      list->head = MEMGetNextListObject(list, list->head);
      list->count--;
      return;
   }

   if (list->tail == object) {
      // Remove from tail
      list->tail = MEMGetPrevListObject(list, list->tail);
      list->count--;
      return;
   }

   do {
      head = MEMGetNextListObject(list, head);
   } while (head && head != object);

   if (head == object) {
      // Remove from middle of list
      auto link = getLink(list, object);
      auto before = link->next;
      auto after = link->prev;
      getLink(list, before)->next = after;
      getLink(list, after)->prev = before;
      list->count--;
   }
}

void *
MEMGetNextListObject(MemoryList *list, void *object)
{
   if (!object) {
      return list->head;
   }

   return getLink(list, object)->next;
}

void *
MEMGetPrevListObject(MemoryList *list, void *object)
{
   if (!object) {
      return list->tail;
   }

   return getLink(list, object)->prev;
}

void *
MEMGetNthListObject(MemoryList *list, uint16_t n)
{
   void *head = nullptr;

   for (auto i = 0u; i < n && head; ++i) {
      head = MEMGetNextListObject(list, head);
   }

   return head;
}

void
CoreInit::registerMemlistFunctions()
{
   RegisterKernelFunction(MEMInitList);
   RegisterKernelFunction(MEMAppendListObject);
   RegisterKernelFunction(MEMPrependListObject);
   RegisterKernelFunction(MEMInsertListObject);
   RegisterKernelFunction(MEMRemoveListObject);
   RegisterKernelFunction(MEMGetNextListObject);
   RegisterKernelFunction(MEMGetPrevListObject);
   RegisterKernelFunction(MEMGetNthListObject);
}