#include "coreinit.h"
#include "coreinit_memlist.h"

namespace coreinit
{

static MEMListLink *
getLink(MEMList *list,
        void *object)
{
   return reinterpret_cast<MEMListLink*>(reinterpret_cast<uint8_t*>(object) + list->offsetToMEMListLink);
}

void
setFirstObject(MEMList *list,
               void *object)
{
   auto link = getLink(list, object);
   list->head = object;
   list->tail = object;
   link->next = nullptr;
   link->prev = nullptr;
   list->count = 1;
}

void
MEMInitList(MEMList *list,
            uint16_t offsetToMEMListLink)
{
   list->head = nullptr;
   list->tail = nullptr;
   list->count = 0;
   list->offsetToMEMListLink = offsetToMEMListLink;
}

void
MEMAppendListObject(MEMList *list,
                    void *object)
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
   list->tail = object;
   list->count++;
}

void
MEMPrependListObject(MEMList *list,
                     void *object)
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
MEMInsertListObject(MEMList *list,
                    void *before,
                    void *object)
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
MEMRemoveListObject(MEMList *list,
                    void *object)
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

      if (list->head) {
         getLink(list, list->head)->prev = nullptr;
      }

      list->count--;
      return;
   }

   if (list->tail == object) {
      // Remove from tail
      list->tail = MEMGetPrevListObject(list, list->tail);

      if (list->tail) {
         getLink(list, list->tail)->next = nullptr;
      }

      list->count--;
      return;
   }

   do {
      head = MEMGetNextListObject(list, head);
   } while (head && head != object);

   if (head == object) {
      // Remove from middle of list
      auto link = getLink(list, object);
      auto next = link->next;
      auto prev = link->prev;
      getLink(list, prev)->next = next;
      getLink(list, next)->prev = prev;
      list->count--;
   }
}

void *
MEMGetNextListObject(MEMList *list,
                     void *object)
{
   if (!object) {
      return list->head;
   }

   return getLink(list, object)->next;
}

void *
MEMGetPrevListObject(MEMList *list,
                     void *object)
{
   if (!object) {
      return list->tail;
   }

   return getLink(list, object)->prev;
}

void *
MEMGetNthListObject(MEMList *list,
                    uint16_t n)
{
   auto head = list->head;

   for (auto i = 0u; i < n && head; ++i) {
      head = MEMGetNextListObject(list, head);
   }

   return head;
}

void
Module::registerMemlistFunctions()
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

} // namespace coreinit