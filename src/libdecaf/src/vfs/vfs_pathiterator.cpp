#include "vfs_path.h"
#include "vfs_pathiterator.h"

#include <algorithm>

namespace vfs
{

PathIterator::PathIterator(const Path *path,
                           std::string::const_iterator position) :
   mPath(path),
   mPosition(position),
   mElement()
{
}

PathIterator::PathIterator(const Path *path,
                           std::string::const_iterator position,
                           std::string_view element) :
   mPath(path),
   mPosition(position),
   mElement(element)
{
}

PathIterator::reference PathIterator::operator *() const
{
   return mElement;
}

const Path *PathIterator::operator ->() const
{
   return &mElement;
}

PathIterator &PathIterator::operator++()
{
   auto pathBegin = mPath->mPath.begin();
   auto pathEnd = mPath->mPath.end();
   mPosition += mElement.mPath.size();

   if (mPosition == pathEnd) {
      mElement.clear();
      return *this;
   } else {
      if (*mPosition == '/') {
         mPosition++;
      }

      auto elementStart = mPosition;
      auto elementEnd = std::find(mPosition, pathEnd, '/');
      mElement.mPath.assign(elementStart, elementEnd);
   }

   return *this;
}

PathIterator
PathIterator::operator++(int)
{
   auto prev = *this;
   ++*this;
   return prev;
}

PathIterator &
PathIterator::operator--()
{
   auto pathBegin = mPath->mPath.begin();
   auto pathEnd = mPath->mPath.end();
   auto elementEnd = (mPosition == pathEnd) ? (pathEnd) : (mPosition - 1);
   mPosition = elementEnd - 1;

   while (mPosition > pathBegin) {
      if (*mPosition == '/') {
         ++mPosition;
         break;
      }

      --mPosition;
   }

   mElement.mPath.assign(mPosition, elementEnd);
   return *this;
}

PathIterator
PathIterator::operator--(int)
{
   auto prev = *this;
   --*this;
   return prev;
}

} // namespace vfs
