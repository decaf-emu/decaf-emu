#pragma once
#include "vfs_path.h"

#include <cstddef>
#include <iterator>

namespace vfs
{

class PathIterator
{
   friend class Path;

public:
   using iterator_category = std::bidirectional_iterator_tag;
   using value_type = Path;
   using difference_type = std::ptrdiff_t;
   using pointer = const Path *;
   using reference = const Path &;

   PathIterator() = default;
   PathIterator(const Path *path,
                std::string::const_iterator position);
   PathIterator(const Path *path,
                std::string::const_iterator position,
                std::string_view element);

   PathIterator(const PathIterator &) = default;
   PathIterator(PathIterator &&) = default;
   PathIterator &operator=(const PathIterator &) = default;
   PathIterator &operator=(PathIterator &&) = default;

   reference operator *() const;
   const Path *operator ->() const;

   PathIterator &operator++();
   PathIterator operator++(int);

   PathIterator &operator--();
   PathIterator operator--(int);

   friend bool operator ==(const PathIterator &lhs, const PathIterator &rhs)
   {
      return lhs.mPosition == rhs.mPosition;
   }

   friend bool operator !=(const PathIterator &lhs, const PathIterator &rhs)
   {
      return lhs.mPosition != rhs.mPosition;
   }

private:
   const Path *mPath = nullptr;
   std::string::const_iterator mPosition;
   Path mElement;
};

PathIterator begin(Path &path);
PathIterator end(Path &path);

} // namespace vfs
