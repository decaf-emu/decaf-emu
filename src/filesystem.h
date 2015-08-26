#pragma once
#include <algorithm>
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <vector>

struct DirectoryEntry
{
   enum Type
   {
      Directory,
      File
   };

   Type type;
   std::string name;
   size_t size;
};

class DirectoryIterator
{
public:
   virtual ~DirectoryIterator() { }
   virtual void rewind() = 0;
   virtual bool read(DirectoryEntry &entry) = 0;
};

class IoStream
{
public:
   virtual ~IoStream() { }
   virtual uint32_t tell() = 0;
   virtual void seek(int64_t pos) = 0;
   virtual void skip(size_t n) = 0;
   virtual size_t size() = 0;
   virtual size_t read(void *b, size_t n) = 0;
   virtual void close() = 0;
   virtual bool good() = 0;
};

class FileSystem
{
public:
   enum Mode : int
   {
      Input = std::ios_base::in,
      Output = std::ios_base::out,
      Binary = std::ios_base::binary
   };

   using File = std::unique_ptr<IoStream>;
   using Directory = std::unique_ptr<DirectoryIterator>;

public:
   virtual File openFile(const std::string& path, int mode) = 0;
   virtual Directory openDirectory(const std::string& path) = 0;
};

class HostFileSystem : public FileSystem
{
   class HostFile : public IoStream
   {
   public:
      HostFile(const std::string &path, int mode)
         : mFile(path, mode)
      {
      }

      virtual ~HostFile() override
      {
      }

      size_t size() override
      {
         auto pos = mFile.tellg();
         mFile.seekg(0, std::fstream::end);

         auto size = mFile.tellg();
         mFile.seekg(pos);

         return size;
      }

      uint32_t tell() override
      {
         return static_cast<uint32_t>(mFile.tellg());
      }

      void seek(int64_t pos) override
      {
         if (pos >= 0) {
            mFile.seekg(pos, std::fstream::beg);
         } else {
            mFile.seekg(pos, std::fstream::end);
         }
      }

      void skip(size_t n) override
      {
         mFile.seekg(n, std::fstream::cur);
      }

      size_t read(void *b, size_t n) override
      {
         mFile.read(static_cast<char*>(b), n);
         return mFile.gcount();
      }

      void close() override
      {
         mFile.close();
      }

      bool good() override
      {
         return mFile.good();
      }

   private:
      std::ifstream mFile;

   };

   class HostDirectory : public DirectoryIterator
   {
   public:
      HostDirectory(const std::string &path) :
         mBegin(path)
      {
         rewind();
      }

      virtual ~HostDirectory() override
      {
      }

      void rewind() override
      {
         mIterator = mBegin;
      }

      bool read(DirectoryEntry &entry) override
      {
         if (mIterator == std::experimental::filesystem::directory_iterator()) {
            return false;
         }

         auto status = mIterator->status();
         auto path = mIterator->path();
         entry.name = path.filename().string();

         if (std::experimental::filesystem::is_directory(status)) {
            entry.type = DirectoryEntry::Directory;
            entry.size = 0;
         } else {
            entry.type = DirectoryEntry::File;
            entry.size = std::experimental::filesystem::file_size(path);
         }

         mIterator++;
         return true;
      }

   private:
      std::experimental::filesystem::directory_iterator mBegin;
      std::experimental::filesystem::directory_iterator mIterator;
   };

public:
   HostFileSystem(const std::string &root)
      : mRoot(root)
   {
   }

   FileSystem::File
   openFile(const std::string &path, int mode) override
   {
      return std::make_unique<HostFile>(mRoot + path, mode);
   }

   FileSystem::Directory
   openDirectory(const std::string &path) override
   {
      return std::make_unique<HostDirectory>(mRoot + path);
   }

private:
   std::string mRoot;
};

class VirtualFileSystem : public FileSystem
{
   struct MountPoint
   {
      std::string path;
      std::unique_ptr<FileSystem> fs;
   };

public:
   VirtualFileSystem(std::string path) :
      mPath(path)
   {
   }

   bool
   mount(std::string path, std::unique_ptr<FileSystem> &&fs)
   {
      assert(path.find(mPath) == 0);
      mMounts.emplace_back(MountPoint { std::move(path), std::move(fs) });
      return true;
   }

   bool
   unmount(const std::string &path)
   {
      assert(path.find(mPath) == 0);
      auto itr = std::find_if(mMounts.begin(), mMounts.end(),
                              [&path](auto &mount) {
                                 return mount.path == path;
                              });

      if (itr == mMounts.end()) {
         return false;
      }

      mMounts.erase(itr);
      return true;
   }

   FileSystem::File
   openFile(const std::string& path, int mode) override
   {
      auto mount = findMountPoint(path);

      if (!mount) {
         return nullptr;
      }

      auto relPath = path.substr(mount->path.length());
      return mount->fs->openFile(relPath, mode);
   }

   FileSystem::Directory
   openDirectory(const std::string& path) override
   {
      auto mount = findMountPoint(path);

      if (!mount) {
         return nullptr;
      }

      auto relPath = path.substr(mount->path.length());
      return mount->fs->openDirectory(relPath);
   }

private:
   const MountPoint*
   findMountPoint(const std::string& path) const
   {
      const MountPoint* result = nullptr;

      for (auto &mount : mMounts) {
         if (result && result->path.length() > mount.path.length()) {
            // If this mount has a shorter prefix, then don't bother checking...
            continue;
         }

         if (path.compare(0, mount.path.length(), mount.path) == 0) {
            result = &mount;
         }
      }

      return result;
   }

private:
   std::string mPath;
   std::list<MountPoint> mMounts;
};
