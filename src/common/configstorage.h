#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

template<typename SettingsType>
class ConfigStorage
{
public:
   using ChangeListener = std::function<void(const SettingsType &)>;
   using Settings = SettingsType;

   ConfigStorage() :
      mStorage(std::make_shared<Settings>())
   {
   }

   void set(std::shared_ptr<Settings> settings)
   {
      std::lock_guard<std::mutex> lock { mMutex };
      mStorage = settings;

      for (auto &listener : mListeners) {
         listener(*settings);
      }
   }

   std::shared_ptr<Settings> get()
   {
      std::lock_guard<std::mutex> lock { mMutex };
      return mStorage;
   }

   void addListener(ChangeListener listener)
   {
      std::lock_guard<std::mutex> lock { mMutex };
      mListeners.emplace_back(std::move(listener));
   }

private:
   std::mutex mMutex;
   std::shared_ptr<Settings> mStorage;
   std::vector<ChangeListener> mListeners;
};
