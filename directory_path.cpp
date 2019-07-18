#include "stdafx.h"
#include "directory_path.h"
#include "file_path.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "dirent.h"

DirectoryPath::DirectoryPath(string p) : path(move(p)) {
}

FilePath DirectoryPath::file(const std::string& f) const {
  return FilePath(*this, f);
}

DirectoryPath DirectoryPath::subdirectory(const std::string& s) const {
  return DirectoryPath(path + "/" + s);
}

static bool isDirectory(const string& path) {
  struct stat path_stat;
  if (stat(path.c_str(), &path_stat))
    return false;
  else
    return S_ISDIR(path_stat.st_mode);
}

bool DirectoryPath::exists() const {
  return isDirectory(getPath());
}

void DirectoryPath::createIfDoesntExist() const {
  if (!exists()) {
#ifndef WINDOWS
    USER_CHECK(!mkdir(path.data(), 0750)) << "Unable to create directory \"" + path + "\": " + strerror(errno);
#else
    USER_CHECK(!mkdir(path.data())) << "Unable to create directory \"" + path + "\": " + strerror(errno);
#endif
  }
}

void DirectoryPath::removeRecursively() const {
  if (exists()) {
    for (auto file : getFiles())
      remove(file.getPath());
    for (auto subdir : getSubDirs())
      subdirectory(subdir).removeRecursively();
    rmdir(getPath());
  }
}

static bool isRegularFile(const string& path) {
  struct stat path_stat;
  if (stat(path.c_str(), &path_stat))
    return false;
  else
    return S_ISREG(path_stat.st_mode);
}

vector<FilePath> DirectoryPath::getFiles() const {
  vector<FilePath> ret;
  if (DIR* dir = opendir(path.data())) {
    while (dirent* ent = readdir(dir))
      if (isRegularFile(path + "/" + ent->d_name))
        ret.push_back(FilePath(*this, ent->d_name));
    closedir(dir);
  }
  return ret;
}

vector<string> DirectoryPath::getSubDirs() const {
  vector<string> ret;
  if (DIR* dir = opendir(path.data())) {
    while (dirent* ent = readdir(dir))
      if (isDirectory(path + "/" + ent->d_name) && strcmp(ent->d_name, ".") && strcmp(ent->d_name, ".."))
        ret.push_back(ent->d_name);
    closedir(dir);
  }
  return ret;
}

const char* DirectoryPath::getPath() const {
  return path.data();
}

std::ostream& operator <<(std::ostream& d, const DirectoryPath& path) {
  return d << path.getPath();
}

bool isAbsolutePath(const char* str) {
  // TODO: mac support
#ifdef _WIN32
  if ((str[0] >= 'a' && str[0] <= 'z') || (str[0] >= 'A' && str[0] <= 'Z'))
    if (str[1] == ':' && (str[2] == '/' || str[2] == '\\'))
      return true;
#else
  if (str[0] == '/')
    return true;
#endif
  return false;
}

DirectoryPath DirectoryPath::current() {
  char buffer[2048];
#ifdef _WIN32
  if (!GetCurrentDirectory(sizeof(buf), buf))
    CHECK(false && "GetCurrentDirectory error");
  return string(buf);
#else
  char* name = getcwd(buffer, sizeof(buffer) - 1);
  CHECK(name && "getcwd error");
  return DirectoryPath(name);
#endif
}

bool DirectoryPath::isAbsolute() const {
  return isAbsolutePath(path.c_str());
}

DirectoryPath DirectoryPath::absolute() const {
  if (isAbsolutePath(path.c_str()))
    return *this;
  // TODO: this is not exactly right if paths contain dots (../../)
  return DirectoryPath(current().path + "/" + path);
}
