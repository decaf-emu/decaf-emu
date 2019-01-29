
#define ENUM_BEG(name, type) \
   std::string to_string(name enumValue) { \
      using namespace name##_; \
      switch (enumValue) {

#define ENUM_VALUE(key, value) \
      case key: \
         return #key;

#define ENUM_END(name) \
      default: \
         return std::to_string(static_cast<int>(enumValue)); \
      } \
   }

#define FLAGS_BEG(name, type) \
   std::string to_string(name enumValue) { \
      using namespace name##_; \
      std::string out;

#define FLAGS_VALUE(key, value) \
      if (enumValue & value) { \
         if (out.size()) { out += " | "; } \
         out += #key; \
      }

#define FLAGS_END(name) \
      return out; \
   }
