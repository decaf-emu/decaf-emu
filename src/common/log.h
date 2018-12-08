#pragma once
#include <fmt/format.h>
#include <string_view>

/**
 * Looks and acts like a spdlog logger but without including the spdlog header.
 */
class Logger
{
public:
   enum class Level
   {
      trace = 0,
      debug = 1,
      info = 2,
      warn = 3,
      err = 4,
      critical = 5,
      off = 6
   };

public:
   template<typename String, typename... Args>
   inline void trace(const String &str, const Args & ... args)
   {
      log(Level::trace, fmt::format(str, args...));
   }

   template<typename String, typename... Args>
   inline void debug(const String &str, const Args & ... args)
   {
      log(Level::debug, fmt::format(str, args...));
   }

   template<typename String, typename... Args>
   inline void info(const String &str, const Args & ... args)
   {
      log(Level::info, fmt::format(str, args...));
   }

   template<typename String, typename... Args>
   inline void warn(const String &str, const Args & ... args)
   {
      log(Level::warn, fmt::format(str, args...));
   }

   template<typename String, typename... Args>
   inline void error(const String &str, const Args & ... args)
   {
      log(Level::err, fmt::format(str, args...));
   }

   template<typename String, typename... Args>
   inline void critical(const String &str, const Args & ... args)
   {
      log(Level::critical, fmt::format(str, args...));
   }

   inline void trace(std::string_view msg)
   {
      log(Level::trace, msg);
   }

   inline void debug(std::string_view msg)
   {
      log(Level::debug, msg);
   }

   inline void info(std::string_view msg)
   {
      log(Level::info, msg);
   }

   inline void warn(std::string_view msg)
   {
      log(Level::warn, msg);
   }

   inline void error(std::string_view msg)
   {
      log(Level::err, msg);
   }

   inline void critical(std::string_view msg)
   {
      log(Level::critical, msg);
   }

   Logger &operator=(std::shared_ptr<void> logger)
   {
      mLogger = logger;
      return *this;
   }

   Logger *operator->()
   {
      return this;
   }

   bool should_log(Level level);

private:
   void log(Level lvl, std::string_view msg);

private:
   std::shared_ptr<void> mLogger;
};

extern Logger
gLog;
