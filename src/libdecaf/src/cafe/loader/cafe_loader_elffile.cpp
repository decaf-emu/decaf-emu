#pragma optimize("", off)
#include "cafe_loader_elffile.h"
#include <common/byte_swap.h>

namespace cafe::loader::internal
{

static void
ELFFILE_SwapFileHeader(virt_ptr<rpl::Header> header)
{
   header->type = byte_swap(header->type);
   header->machine = byte_swap(header->machine);
   header->version = byte_swap(header->version);
   header->entry = byte_swap(header->entry);
   header->phoff = byte_swap(header->phoff);
   header->shoff = byte_swap(header->shoff);
   header->flags = byte_swap(header->flags);
   header->ehsize = byte_swap(header->ehsize);
   header->phentsize = byte_swap(header->phentsize);
   header->phnum = byte_swap(header->phnum);
   header->shentsize = byte_swap(header->shentsize);
   header->shnum = byte_swap(header->shnum);
   header->shstrndx = byte_swap(header->shstrndx);
}

static void
ELFFILE_SwapSectionHeader(virt_ptr<rpl::SectionHeader> header)
{
   header->name = byte_swap(header->name);
   header->type = byte_swap(header->type);
   header->flags = byte_swap(header->flags);
   header->addr = byte_swap(header->addr);
   header->offset = byte_swap(header->offset);
   header->size = byte_swap(header->size);
   header->link = byte_swap(header->link);
   header->info = byte_swap(header->info);
   header->addralign = byte_swap(header->addralign);
   header->entsize = byte_swap(header->entsize);
}

int
ELFFILE_ValidateAndPrepareMinELF(virt_ptr<void> chunkBuffer,
                                 size_t chunkSize,
                                 virt_ptr<rpl::Header> *outHeader,
                                 virt_ptr<rpl::SectionHeader> *outSectionHeaders,
                                 uint32_t *outShEntSize,
                                 uint32_t *outPhEntSize)
{
   const auto currentEncoding = rpl::ELFDATA2MSB;
   auto header = virt_cast<rpl::Header *>(chunkBuffer);

   if (chunkSize < 0x104) {
      return 0xBAD00018;
   }

   if (header->magic[0] != 0x7F ||
       header->magic[1] != 'E' ||
       header->magic[2] != 'L' ||
       header->magic[3] != 'F') {
      return 0xBAD00019;
   }

   if (header->fileClass != rpl::ELFCLASS32) {
      return 0xBAD0001A;
   }

   if (header->elfVersion > rpl::EV_CURRENT) {
      return 0xBAD0001B;
   }

   if (header->encoding != currentEncoding) {
      ELFFILE_SwapFileHeader(header);
   }

   if (!header->machine) {
      return 0xBAD0001C;
   }

   if (header->version != 1) {
      return 0xBAD0001D;
   }

   auto ehsize = static_cast<uint32_t>(header->ehsize);
   if (ehsize) {
      if (header->ehsize < sizeof(rpl::Header)) {
         return 0xBAD0001E;
      }
   } else {
      ehsize = static_cast<uint32_t>(sizeof(rpl::Header));
   }

   auto phoff = header->phoff;
   if (phoff && (phoff < ehsize || phoff >= chunkSize)) {
      return 0xBAD0001F;
   }

   auto shoff = header->shoff;
   if (shoff && (shoff < ehsize || shoff >= chunkSize)) {
      return 0xBAD00020;
   }

   if (header->shstrndx && header->shstrndx >= header->shnum) {
      return 0xBAD00021;
   }

   auto phentsize = header->phentsize ?
      static_cast<uint16_t>(header->phentsize) :
      static_cast<uint16_t>(32);
   if (header->phoff &&
      (header->phoff + phentsize * header->phnum) > chunkSize) {
      return 0xBAD00022;
   }

   auto shentsize = header->shentsize ?
      static_cast<uint32_t>(header->shentsize) :
      static_cast<uint32_t>(sizeof(rpl::SectionHeader));
   if (header->shoff &&
      (header->shoff + shentsize * header->shnum) > chunkSize) {
      return 0xBAD00023;
   }

   if (header->encoding != currentEncoding) {
      for (auto i = 1u; i < header->shnum; ++i) {
         ELFFILE_SwapSectionHeader(
            virt_cast<rpl::SectionHeader *>(
               virt_cast<virt_addr>(chunkBuffer) + shoff + (i * shentsize)));
      }
   }

   for (auto i = 1u; i < header->shnum; ++i) {
      auto sectionHeader =
         virt_cast<rpl::SectionHeader *>(
            virt_cast<virt_addr>(chunkBuffer) + shoff + (i * shentsize));

      if (sectionHeader->size &&
          sectionHeader->type != rpl::SHT_NOBITS) {
         if (sectionHeader->offset < ehsize) {
            return 0xBAD00024;
         }

         if (sectionHeader->offset >= shoff &&
             sectionHeader->offset < (shoff + header->shnum * shentsize)) {
            return 0xBAD00027;
         }
      }
   }

   // TODO: loader.elf loads program headers too.

   *outHeader = header;
   *outShEntSize = shentsize;
   *outPhEntSize = phentsize;
   *outSectionHeaders =
      virt_cast<rpl::SectionHeader *>(virt_cast<virt_addr>(chunkBuffer) + shoff);
   return 0;
}

} // namespace cafe::loader::internal
