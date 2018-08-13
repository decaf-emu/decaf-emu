#pragma once
#include <string>
#include <string_view>

namespace fs
{

template<char Separator>
class GenericPath
{
public:
   static const char PathSeparator = Separator;
   static const char ExtensionSeparator = '.';

public:
   class Iterator
   {
   public:
      Iterator(const GenericPath &path, size_t pos) :
         mPath(path.path()),
         mPosStart(pos)
      {
         mPosEnd = mPath.find_first_of(GenericPath::PathSeparator, mPosStart);
      }

      std::string operator*() const
      {
         if (mPosStart == 0 && mPosEnd == 0) {
            return mPath.substr(0, 1);
         } else {
            return mPath.substr(mPosStart, mPosEnd - mPosStart);
         }
      }

      bool operator!=(const Iterator &other) const
      {
         return mPosStart != other.mPosStart;
      }

      Iterator &operator ++()
      {
         mPosStart = mPosEnd;

         if (mPosStart != std::string::npos) {
            mPosStart += 1;
            mPosEnd = mPath.find_first_of(GenericPath::PathSeparator, mPosStart);
         }

         if (mPosStart >= mPath.size()) {
            mPosStart = std::string::npos;
         }

         return *this;
      }

   private:
      const std::string &mPath;
      size_t mPosStart = 0;
      size_t mPosEnd = 0;
   };

public:
   GenericPath()
   {
   }

   template<char OtherSeperator>
   GenericPath(const GenericPath<OtherSeperator> &src) :
      GenericPath(src.path())
   {
   }

   GenericPath(const std::string &src) :
      GenericPath(std::string_view { src })
   {
   }

   GenericPath(const char *src) :
      GenericPath(std::string_view { src })
   {
   }

   GenericPath(std::string_view src)
   {
      static const char ParentSeparator[4] = { Separator, '.', '.', 0 };
      mPath = src;

      // Normalise path separators
      for (auto pos = mPath.find_first_of("\\/"); pos != std::string::npos; pos = mPath.find_first_of("\\/", pos + 1)) {
         mPath[pos] = PathSeparator;
      }

      // Simplify parent .. operator (hello/../world -> world)
      for (auto pos = mPath.find(ParentSeparator, 0); pos != std::string::npos; ) {
         auto parentEnd = pos;

         if (parentEnd == 0) {
            // A valid path will never start with /.. but we will contract it to / anyway...
            mPath.erase(pos + 1, 3);
            pos = mPath.find(ParentSeparator, pos + 1);
            continue;
         }

         auto parentStart = mPath.rfind(PathSeparator, parentEnd - 1);

         if (parentStart == std::string::npos) {
            parentStart = 0;
         } else {
            parentStart += 1;
         }

         if (mPath[parentStart] != '.' && mPath[parentStart + 1] != '.') {
            mPath.erase(parentStart, parentEnd - parentStart + 4);

            if (parentStart != 0) {
               parentStart -= 1;
            }

            pos = mPath.find(ParentSeparator, parentStart);
         } else {
            pos = mPath.find(ParentSeparator, pos + 3);
         }
      }

      // Simplify multiple path separators (hello//world -> hello/world)
      for (auto pos = mPath.find_first_of(PathSeparator, 0); pos != std::string::npos; ) {
         auto end = mPath.find_first_not_of(PathSeparator, pos);

         if (end != pos + 1) {
            if (end != std::string::npos) {
               // Erase up to next non / character
               mPath.erase(pos, end - pos - 1);
            } else if (pos != mPath.size() - 1) {
               // Erase trailing multiple /
               mPath.erase(pos + 1);
            }
         }

         pos = mPath.find_first_of(PathSeparator, end);
      }
   }

   GenericPath
   join(const GenericPath &other) const
   {
      return { mPath + PathSeparator + other.mPath };
   }

   template<char OtherSeparator>
   GenericPath
   join(const GenericPath<OtherSeparator> &other) const
   {
      return { mPath + PathSeparator + other.path() };
   }

   const std::string &
   path() const
   {
      return mPath;
   }

   std::string
   filename() const
   {
      auto pos = mPath.find_last_of(PathSeparator);

      if (pos == std::string::npos) {
         return mPath;
      } else {
         return mPath.substr(pos + 1);
      }
   }

   std::string
   extension() const
   {
      auto fn = filename();
      auto pos = fn.find_last_of(ExtensionSeparator);

      if (pos == std::string::npos) {
         return {};
      } else {
         return fn.substr(pos + 1);
      }
   }

   std::string
   parentPath() const
   {
      auto pos = mPath.find_last_of(PathSeparator);

      if (pos == 0) {
         return "/";
      } else if (pos == std::string::npos) {
         return std::string {};
      } else {
         return mPath.substr(0, pos);
      }
   }

   bool
   isRelativePath() const
   {
      if (!mPath.size()) {
         return false;
      }

      if (PathSeparator == '/' && mPath.size() > 0 && mPath[0] == '/') {
         return false;
      }

      if (PathSeparator == '\\' && mPath.size() > 2 && mPath[1] == ':' && mPath[2] == '\\') {
         return false;
      }

      return true;
   }

   Iterator
   begin() const
   {
      return Iterator { *this, 0 };
   }

   Iterator
   end() const
   {
      return Iterator { *this, std::string::npos };
   }

private:
   std::string mPath;
};

using Path = GenericPath<'/'>;

} // namespace fs
