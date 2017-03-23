#pragma once
#include <vector>
#include "filesystem_node.h"
#include "filesystem_folderhandle.h"

namespace fs
{

struct VirtualFolderHandle : public IFolderHandle
{
   using iterator = std::vector<Node *>::const_iterator;

public:
   VirtualFolderHandle(const iterator &begin,
                       const iterator &end) :
      mBegin(begin),
      mEnd(end),
      mIterator(mBegin)
   {
   }

   virtual ~VirtualFolderHandle() override = default;

   virtual bool
   open() override
   {
      mIterator = mBegin;
      return true;
   }

   virtual void
   close() override
   {
      mIterator = mEnd;
   }

   virtual bool
   read(FolderEntry &entry) override
   {
      if (mIterator == mEnd) {
         return false;
      }

      auto node = *mIterator;
      entry.name = node->name();
      entry.size = node->size();

      if (node->type() == Node::FileNode) {
         entry.type = FolderEntry::File;
      } else if (node->type() == Node::FolderNode) {
         entry.type = FolderEntry::Folder;
      } else {
         entry.type = FolderEntry::Unknown;
      }

      mIterator++;
      return true;
   }

   virtual bool
   rewind() override
   {
      mIterator = mBegin;
      return true;
   }

private:
   const iterator mBegin;
   const iterator mEnd;
   iterator mIterator;
};

} // namespace fs
