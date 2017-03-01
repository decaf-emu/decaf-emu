/*! \file json.hpp
\brief JSON input and output archives */
/*
Copyright (c) 2014, Randolph Voorhies, Shane Grant
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of cereal nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL RANDOLPH VOORHIES OR SHANE GRANT BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef CEREAL_ARCHIVES_OPTIONAL_JSON_HPP_
#define CEREAL_ARCHIVES_OPTIONAL_JSON_HPP_

#include <cereal/archives/json.hpp>

namespace cereal
{

/**
 * This is similar to JSONInputArchive, except it allows all values to be optional
 * and passes the kParseTrailingCommasFlag and kParseCommentsFlag flags to rapidjson
 * which makes the json format a little less strict.
 *
 * If a value is not found in the json it will just not update the value, so you must
 * make sure you initialise the config value with default values before reading json.
 */
class JSONOptionalInputArchive : public InputArchive<JSONOptionalInputArchive>, public traits::TextArchive
{
private:
   using ReadStream = CEREAL_RAPIDJSON_NAMESPACE::IStreamWrapper;
   typedef CEREAL_RAPIDJSON_NAMESPACE::GenericValue<CEREAL_RAPIDJSON_NAMESPACE::UTF8<>> JSONValue;
   typedef JSONValue::ConstMemberIterator MemberIterator;
   typedef JSONValue::ConstValueIterator ValueIterator;
   typedef CEREAL_RAPIDJSON_NAMESPACE::Document::GenericValue GenericValue;

public:
   /*! @name Common Functionality
   Common use cases for directly interacting with an JSONOptionalInputArchive */
   //! @{

   //! Construct, reading from the provided stream
   /*! @param stream The stream to read from */
   JSONOptionalInputArchive(std::istream & stream) :
      InputArchive<JSONOptionalInputArchive>(this),
      itsNextName(nullptr),
      itsReadStream(stream)
   {
      itsDocument.ParseStream<
         CEREAL_RAPIDJSON_NAMESPACE::kParseCommentsFlag |
         CEREAL_RAPIDJSON_NAMESPACE::kParseTrailingCommasFlag |
         CEREAL_RAPIDJSON_NAMESPACE::kParseFullPrecisionFlag |
         CEREAL_RAPIDJSON_NAMESPACE::kParseNanAndInfFlag>(itsReadStream);
      if (itsDocument.IsArray())
         itsIteratorStack.emplace_back(itsDocument.Begin(), itsDocument.End());
      else
         itsIteratorStack.emplace_back(itsDocument.MemberBegin(), itsDocument.MemberEnd());
   }

   ~JSONOptionalInputArchive() CEREAL_NOEXCEPT = default;

   //! Loads some binary data, encoded as a base64 string
   /*! This will automatically start and finish a node to load the data, and can be called directly by
   users.

   Note that this follows the same ordering rules specified in the class description in regards
   to loading in/out of order */
   void loadBinaryValue(void * data, size_t size, const char * name = nullptr)
   {
      itsNextName = name;

      std::string encoded;
      loadValue(encoded);
      auto decoded = base64::decode(encoded);

      if (size != decoded.size())
         throw Exception("Decoded binary data size does not match specified size");

      std::memcpy(data, decoded.data(), decoded.size());
      itsNextName = nullptr;
   };

private:
   //! @}
   /*! @name Internal Functionality
   Functionality designed for use by those requiring control over the inner mechanisms of
   the JSONOptionalInputArchive */
   //! @{

   //! An internal iterator that handles both array and object types
   /*! This class is a variant and holds both types of iterators that
   rapidJSON supports - one for arrays and one for objects. */
   class Iterator
   {
   public:
      Iterator() : itsIndex(0), itsType(Null_)
      {
      }

      Iterator(MemberIterator begin, MemberIterator end) :
         itsMemberItBegin(begin), itsMemberItEnd(end), itsIndex(0), itsType(Member)
      {
         if (std::distance(begin, end) == 0)
            itsType = Null_;
      }

      Iterator(ValueIterator begin, ValueIterator end) :
         itsValueItBegin(begin), itsValueItEnd(end), itsIndex(0), itsType(Value)
      {
         if (std::distance(begin, end) == 0)
            itsType = Null_;
      }

      //! Advance to the next node
      Iterator & operator++()
      {
         ++itsIndex;
         return *this;
      }

      //! Get the value of the current node
      GenericValue const & value()
      {
         switch (itsType) {
         case Value: return itsValueItBegin[itsIndex];
         case Member: return itsMemberItBegin[itsIndex].value;
         default: throw cereal::Exception("JSONInputArchive internal error: null or empty iterator to object or array!");
         }
      }

      //! Get the name of the current node, or nullptr if it has no name
      const char * name() const
      {
         if (itsType == Member && (itsMemberItBegin + itsIndex) != itsMemberItEnd)
            return itsMemberItBegin[itsIndex].name.GetString();
         else
            return nullptr;
      }

      //! Adjust our position such that we are at the node with the given name
      /*! @throws Exception if no such named node exists */
      inline bool search(const char * searchName)
      {
         const auto len = std::strlen(searchName);
         size_t index = 0;
         for (auto it = itsMemberItBegin; it != itsMemberItEnd; ++it, ++index) {
            const auto currentName = it->name.GetString();
            if ((std::strncmp(searchName, currentName, len) == 0) &&
               (std::strlen(currentName) == len)) {
               itsIndex = index;
               return true;
            }
         }

         return false;
      }

   private:
      MemberIterator itsMemberItBegin, itsMemberItEnd; //!< The member iterator (object)
      ValueIterator itsValueItBegin, itsValueItEnd;    //!< The value iterator (array)
      size_t itsIndex;                                 //!< The current index of this iterator
      enum Type {Value, Member, Null_} itsType;        //!< Whether this holds values (array) or members (objects) or nothing
   };

   //! Searches for the expectedName node if it doesn't match the actualName
   /*! This needs to be called before every load or node start occurs.  This function will
   check to see if an NVP has been provided (with setNextName) and if so, see if that name matches the actual
   next name given.  If the names do not match, it will search in the current level of the JSON for that name.
   If the name is not found, an exception will be thrown.

   Resets the NVP name after called.

   @throws Exception if an expectedName is given and not found */
   inline bool search()
   {
      bool result = true;

      // The name an NVP provided with setNextName()
      if (itsNextName) {
         // The actual name of the current node
         auto const actualName = itsIteratorStack.back().name();

         // Do a search if we don't see a name coming up, or if the names don't match
         if (!actualName || std::strcmp(itsNextName, actualName) != 0) {
            result = itsIteratorStack.back().search(itsNextName);
         }
      }

      itsNextName = nullptr;
      return result;
   }

public:
   //! Starts a new node, going into its proper iterator
   /*! This places an iterator for the next node to be parsed onto the iterator stack.  If the next
   node is an array, this will be a value iterator, otherwise it will be a member iterator.

   By default our strategy is to start with the document root node and then recursively iterate through
   all children in the order they show up in the document.
   We don't need to know NVPs to do this; we'll just blindly load in the order things appear in.

   If we were given an NVP, we will search for it if it does not match our the name of the next node
   that would normally be loaded.  This functionality is provided by search(). */
   void startNode()
   {
      search();

      if (itsIteratorStack.back().value().IsArray())
         itsIteratorStack.emplace_back(itsIteratorStack.back().value().Begin(), itsIteratorStack.back().value().End());
      else
         itsIteratorStack.emplace_back(itsIteratorStack.back().value().MemberBegin(), itsIteratorStack.back().value().MemberEnd());
   }

   //! Finishes the most recently started node
   void finishNode()
   {
      itsIteratorStack.pop_back();
      ++itsIteratorStack.back();
   }

   //! Retrieves the current node name
   /*! @return nullptr if no name exists */
   const char * getNodeName() const
   {
      return itsIteratorStack.back().name();
   }

   //! Sets the name for the next node created with startNode
   void setNextName(const char * name)
   {
      itsNextName = name;
   }

   //! Loads a value from the current node - small signed overload
   template <class T, traits::EnableIf<std::is_signed<T>::value,
      sizeof(T) < sizeof(int64_t)> = traits::sfinae> inline
      void loadValue(T & val)
   {
      search();

      val = static_cast<T>(itsIteratorStack.back().value().GetInt());
      ++itsIteratorStack.back();
   }

   //! Loads a value from the current node - small unsigned overload
   template <class T, traits::EnableIf<std::is_unsigned<T>::value,
      sizeof(T) < sizeof(uint64_t),
      !std::is_same<bool, T>::value> = traits::sfinae> inline
      void loadValue(T & val)
   {
      search();

      val = static_cast<T>(itsIteratorStack.back().value().GetUint());
      ++itsIteratorStack.back();
   }

   //! Loads a value from the current node - bool overload
   void loadValue(bool & val)
   {
      if (search()) {
         val = itsIteratorStack.back().value().GetBool();
      }

      ++itsIteratorStack.back();
   }

   //! Loads a value from the current node - int64 overload
   void loadValue(int64_t & val)
   {
      if (search()) {
         val = itsIteratorStack.back().value().GetInt64();
      }

      ++itsIteratorStack.back();
   }

   //! Loads a value from the current node - uint64 overload
   void loadValue(uint64_t & val)
   {
      if (search()) {
         val = itsIteratorStack.back().value().GetUint64();
      }

      ++itsIteratorStack.back();
   }

   //! Loads a value from the current node - float overload
   void loadValue(float & val)
   {
      if (search()) {
         val = static_cast<float>(itsIteratorStack.back().value().GetDouble());
      }

      ++itsIteratorStack.back();
   }

   //! Loads a value from the current node - double overload
   void loadValue(double & val)
   {
      if (search()) {
         val = itsIteratorStack.back().value().GetDouble();
      }

      ++itsIteratorStack.back();
   }

   //! Loads a value from the current node - string overload
   void loadValue(std::string & val)
   {
      if (search()) {
         val = itsIteratorStack.back().value().GetString();
      }

      ++itsIteratorStack.back();
   }

   //! Loads a nullptr from the current node
   void loadValue(std::nullptr_t&)
   {
      if (search()) {
         CEREAL_RAPIDJSON_ASSERT(itsIteratorStack.back().value().IsNull());
      }

      ++itsIteratorStack.back();
   }

   // Special cases to handle various flavors of long, which tend to conflict with
   // the int32_t or int64_t on various compiler/OS combinations.  MSVC doesn't need any of this.
#ifndef _MSC_VER
private:
   //! 32 bit signed long loading from current node
   template <class T> inline
      typename std::enable_if<sizeof(T) == sizeof(std::int32_t) && std::is_signed<T>::value, void>::type
      loadLong(T & l)
   {
      loadValue(reinterpret_cast<std::int32_t&>(l));
   }

   //! non 32 bit signed long loading from current node
   template <class T> inline
      typename std::enable_if<sizeof(T) == sizeof(std::int64_t) && std::is_signed<T>::value, void>::type
      loadLong(T & l)
   {
      loadValue(reinterpret_cast<std::int64_t&>(l));
   }

   //! 32 bit unsigned long loading from current node
   template <class T> inline
      typename std::enable_if<sizeof(T) == sizeof(std::uint32_t) && !std::is_signed<T>::value, void>::type
      loadLong(T & lu)
   {
      loadValue(reinterpret_cast<std::uint32_t&>(lu));
   }

   //! non 32 bit unsigned long loading from current node
   template <class T> inline
      typename std::enable_if<sizeof(T) == sizeof(std::uint64_t) && !std::is_signed<T>::value, void>::type
      loadLong(T & lu)
   {
      loadValue(reinterpret_cast<std::uint64_t&>(lu));
   }

public:
   //! Serialize a long if it would not be caught otherwise
   template <class T> inline
      typename std::enable_if<std::is_same<T, long>::value &&
      sizeof(T) >= sizeof(std::int64_t) &&
      !std::is_same<T, std::int64_t>::value, void>::type
      loadValue(T & t)
   {
      loadLong(t);
   }

   //! Serialize an unsigned long if it would not be caught otherwise
   template <class T> inline
      typename std::enable_if<std::is_same<T, unsigned long>::value &&
      sizeof(T) >= sizeof(std::uint64_t) &&
      !std::is_same<T, std::uint64_t>::value, void>::type
      loadValue(T & t)
   {
      loadLong(t);
   }
#endif // _MSC_VER

private:
   //! Convert a string to a long long
   void stringToNumber(std::string const & str, long long & val)
   {
      val = std::stoll(str);
   }
   //! Convert a string to an unsigned long long
   void stringToNumber(std::string const & str, unsigned long long & val)
   {
      val = std::stoull(str);
   }
   //! Convert a string to a long double
   void stringToNumber(std::string const & str, long double & val)
   {
      val = std::stold(str);
   }

public:
   //! Loads a value from the current node - long double and long long overloads
   template <class T, traits::EnableIf<std::is_arithmetic<T>::value,
      !std::is_same<T, long>::value,
      !std::is_same<T, unsigned long>::value,
      !std::is_same<T, std::int64_t>::value,
      !std::is_same<T, std::uint64_t>::value,
      (sizeof(T) >= sizeof(long double) || sizeof(T) >= sizeof(long long))> = traits::sfinae>
      inline void loadValue(T & val)
   {
      std::string encoded;
      loadValue(encoded);

      if (!encoded.empty()) {
         stringToNumber(encoded, val);
      }
   }

   //! Loads the size for a SizeTag
   void loadSize(size_type & size)
   {
      if (itsIteratorStack.size() == 1)
         size = itsDocument.Size();
      else
         size = (itsIteratorStack.rbegin() + 1)->value().Size();
   }

   //! @}

private:
   const char * itsNextName;               //!< Next name set by NVP
   ReadStream itsReadStream;               //!< Rapidjson write stream
   std::vector<Iterator> itsIteratorStack; //!< 'Stack' of rapidJSON iterators
   CEREAL_RAPIDJSON_NAMESPACE::Document itsDocument; //!< Rapidjson document
};

// ######################################################################
// JSONArchive prologue and epilogue functions
// ######################################################################

//! Prologue for NVPs for JSON archives
template <class T> inline
void prologue(JSONOptionalInputArchive &, NameValuePair<T> const &)
{
}

//! Epilogue for NVPs for JSON archives
/*! NVPs do not start or finish nodes - they just set up the names */
template <class T> inline
void epilogue(JSONOptionalInputArchive &, NameValuePair<T> const &)
{
}

// ######################################################################
//! Prologue for SizeTags for JSON archives
/*! SizeTags are strictly ignored for JSON, they just indicate
that the current node should be made into an array */
template <class T> inline
void prologue(JSONOptionalInputArchive &, SizeTag<T> const &)
{
}

// ######################################################################
//! Epilogue for SizeTags for JSON archives
/*! SizeTags are strictly ignored for JSON */
template <class T> inline
void epilogue(JSONOptionalInputArchive &, SizeTag<T> const &)
{
}

// ######################################################################
//! Prologue for all other types for JSON archives (except minimal types)
/*! Starts a new node, named either automatically or by some NVP,
that may be given data by the type about to be archived

Minimal types do not start or finish nodes */
template <class T, traits::EnableIf<!std::is_arithmetic<T>::value,
   !traits::has_minimal_base_class_serialization<T, traits::has_minimal_input_serialization, JSONOptionalInputArchive>::value,
   !traits::has_minimal_input_serialization<T, JSONOptionalInputArchive>::value> = traits::sfinae>
   inline void prologue(JSONOptionalInputArchive & ar, T const &)
{
   ar.startNode();
}

// ######################################################################
//! Epilogue for all other types other for JSON archives (except minimal types)
/*! Finishes the node created in the prologue

Minimal types do not start or finish nodes */
template <class T, traits::EnableIf<!std::is_arithmetic<T>::value,
   !traits::has_minimal_base_class_serialization<T, traits::has_minimal_input_serialization, JSONOptionalInputArchive>::value,
   !traits::has_minimal_input_serialization<T, JSONOptionalInputArchive>::value> = traits::sfinae>
   inline void epilogue(JSONOptionalInputArchive & ar, T const &)
{
   ar.finishNode();
}

// ######################################################################
//! Prologue for arithmetic types for JSON archives
inline
void prologue(JSONOptionalInputArchive &, std::nullptr_t const &)
{
}

// ######################################################################
//! Epilogue for arithmetic types for JSON archives
inline
void epilogue(JSONOptionalInputArchive &, std::nullptr_t const &)
{
}

// ######################################################################
//! Prologue for arithmetic types for JSON archives
template <class T, traits::EnableIf<std::is_arithmetic<T>::value> = traits::sfinae> inline
void prologue(JSONOptionalInputArchive &, T const &)
{
}

// ######################################################################
//! Epilogue for arithmetic types for JSON archives
template <class T, traits::EnableIf<std::is_arithmetic<T>::value> = traits::sfinae> inline
void epilogue(JSONOptionalInputArchive &, T const &)
{
}

// ######################################################################
//! Prologue for strings for JSON archives
template<class CharT, class Traits, class Alloc> inline
void prologue(JSONOptionalInputArchive &, std::basic_string<CharT, Traits, Alloc> const &)
{
}

// ######################################################################
//! Epilogue for strings for JSON archives
template<class CharT, class Traits, class Alloc> inline
void epilogue(JSONOptionalInputArchive &, std::basic_string<CharT, Traits, Alloc> const &)
{
}

// ######################################################################
// Common JSONArchive serialization functions
// ######################################################################
//! Serializing NVP types to JSON
template <class T> inline
void CEREAL_LOAD_FUNCTION_NAME(JSONOptionalInputArchive & ar, NameValuePair<T> & t)
{
   ar.setNextName(t.name);
   ar(t.value);
}

//! Loading arithmetic from JSON
inline
void CEREAL_LOAD_FUNCTION_NAME(JSONOptionalInputArchive & ar, std::nullptr_t & t)
{
   ar.loadValue(t);
}

//! Loading arithmetic from JSON
template <class T, traits::EnableIf<std::is_arithmetic<T>::value> = traits::sfinae> inline
void CEREAL_LOAD_FUNCTION_NAME(JSONOptionalInputArchive & ar, T & t)
{
   ar.loadValue(t);
}

//! loading string from JSON
template<class CharT, class Traits, class Alloc> inline
void CEREAL_LOAD_FUNCTION_NAME(JSONOptionalInputArchive & ar, std::basic_string<CharT, Traits, Alloc> & str)
{
   ar.loadValue(str);
}

// ######################################################################
//! Loading SizeTags from JSON
template <class T> inline
void CEREAL_LOAD_FUNCTION_NAME(JSONOptionalInputArchive & ar, SizeTag<T> & st)
{
   ar.loadSize(st.size);
}
} // namespace cereal

  // register archives for polymorphic support
CEREAL_REGISTER_ARCHIVE(cereal::JSONOptionalInputArchive)

// tie input and output archives together
// CEREAL_SETUP_ARCHIVE_TRAITS(cereal::JSONOptionalInputArchive, cereal::JSONOutputArchive)

#endif // CEREAL_ARCHIVES_JSON_HPP_
