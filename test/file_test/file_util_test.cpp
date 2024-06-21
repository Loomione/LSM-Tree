#include "util/file_util.hh"
#include "gtest/gtest.h"

TEST(FileUtil, CreatFile) {
  std::string dir = "/home/swarm/spectrum_sensing/gsj/lsm/build/test/files";
  {
    auto rc = lsm_tree::file_manager::Create(dir, lsm_tree::FileOptions::DIR_);
    EXPECT_EQ(rc, lsm_tree::RC::OK);
  }
  auto path = dir + "/test.txt";
  {
    auto rc = lsm_tree::file_manager::Create(path, lsm_tree::FileOptions::FILE_);
    EXPECT_EQ(rc, lsm_tree::RC::OK);
  }
  {
    auto rc = lsm_tree::file_manager::Exists(path);
    EXPECT_EQ(rc, true);
  }
  {
    auto rc = lsm_tree::file_manager::IsDirectory(path);
    EXPECT_EQ(rc, false);
  }
  {
    auto rc = lsm_tree::file_manager::IsDirectory(dir);
    EXPECT_EQ(rc, true);
  }
}