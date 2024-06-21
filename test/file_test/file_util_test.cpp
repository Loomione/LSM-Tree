#include "gtest/gtest.h"
#include "util/file_util.hh"

TEST(FileUtil, CreatFile) {
  std::string path = "test/file_test/test_file";
  auto        rc   = lsm_tree::file_manager::Create(path, lsm_tree::FileOptions::FILE_);
  EXPECT_EQ(rc, lsm_tree::RC::OK);
}