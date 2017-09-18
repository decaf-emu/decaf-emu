
#define ENUM_BEG(name, type) \
   std::string enumAsString(name enumValue) { \
      using namespace name##_; \
      switch (enumValue) {

#define ENUM_VALUE(key, value) \
      case key: \
         return #key;

#define ENUM_END(name) \
      default: \
         return std::to_string(static_cast<int>(enumValue)); \
      } \
   } \
   std::ostream &operator<<(std::ostream& os, name enumValue) { \
      return os << enumAsString(enumValue); \
   }

#define FLAGS_BEG(name, type) \
   std::string enumAsString(name enumValue) { \
      using namespace name##_; \
      std::string out;

#define FLAGS_VALUE(key, value) \
      if (enumValue & value) { \
         if (out.size()) { out += " | "; } \
         out += #key; \
      }

#define FLAGS_END(name) \
      return out; \
   } \
   std::ostream &operator<<(std::ostream& os, name enumValue) { \
      return os << enumAsString(enumValue); \
   }
