#include "util/file_util.hh"
#include <memory>
#include <string>
#include "gtest/gtest.h"

using namespace lsm_tree;
using namespace std;
TEST(FileUtil, DISABLED_CreatFile) {
  string dir = "/home/swarm/spectrum_sensing/gsj/lsm/build/test/files";
  {
    auto rc =  FileManager::Create(dir, FileOptions::DIR_);
    EXPECT_EQ(rc, RC::OK);
  }
  auto path = dir + "/test.txt";
  {
    auto rc =  FileManager::Create(path, FileOptions::FILE_);
    EXPECT_EQ(rc, RC::OK);
  }
  {
    auto rc =  FileManager::Exists(path);
    EXPECT_EQ(rc, true);
  }
  {
    auto rc =  FileManager::IsDirectory(path);
    EXPECT_EQ(rc, false);
  }
  {
    auto rc =  FileManager::IsDirectory(dir);
    EXPECT_EQ(rc, true);
  }
}

TEST(FileUtil, FixFileName) {
  string path = "~/spectrum_sensing/gsj/lsm/build/test/files/test.txt";
  auto   str  =  FileManager::FixFileName(path);
  cout << str << endl;
}

TEST(FileUtil, FixDirName) {
  string path = "~/spectrum_sensing/gsj/lsm/build/test/files";
  auto   str  =  FileManager::FixDirName(path);
  cout << str << endl;
  string path2 = "";
  auto   str2  =  FileManager::FixDirName(path2);
  EXPECT_EQ(str2, ".");
}

TEST(FileUtil, GetFileSize) {
  string path = "/home/swarm/spectrum_sensing/gsj/lsm/source/util/file_util.cpp";
  size_t size = 0;
  auto   rc   =  FileManager::GetFileSize(path, size);
  cout << size << endl;
}

TEST(FileUtil, WritAbleFile) {
  string                   path = "/home/swarm/spectrum_sensing/gsj/lsm/build/test/files/test.txt";
  unique_ptr<WritAbleFile> file;
  auto                     rc =  FileManager::OpenWritAbleFile(path, file);
  EXPECT_EQ(rc, RC::OK);
  string data = "hello world\n";
  string data2 = "你好，世界\n";
  rc = file->Append(data);
  EXPECT_EQ(rc, RC::OK);
  rc = file->Append(data2);
  EXPECT_EQ(rc, RC::OK);
  rc = file->Close();
  EXPECT_EQ(rc, RC::OK);

  auto file_path = file->GetPath();
  EXPECT_EQ(file_path, path);
}


TEST(FileUtil, TempFile) {
  string                   dir = "/home/swarm/spectrum_sensing/gsj/lsm/build/test/files";
  string                   subfix = ".tmp";
  unique_ptr<TempFile>     file;
  auto                     rc =  FileManager::OpenTempFile(dir, subfix, file);
  EXPECT_EQ(rc, RC::OK);
  string data = "hello world\n";
  string data2 = "你好，世界\n";
  rc = file->Append(data);
  EXPECT_EQ(rc, RC::OK);
  rc = file->Append(data2);
  EXPECT_EQ(rc, RC::OK);
  rc = file->Close();
  EXPECT_EQ(rc, RC::OK);

  auto file_path = file->GetPath();
  EXPECT_EQ(file_path.find(dir), 0);
}