#pragma once

#include <fmt/ostream.h>
#include <openssl/sha.h>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>
#include "keys.hh"
#include "return_code.hh"

namespace lsm_tree {

using std::ostream;
using std::string;
using std::string_view;
using std::vector;

enum class FileOptions {
  DIR_,
  FILE_,
};

class WritAbleFile;
class TempFile;
class MmapReadAbleFile;
class RandomAccessFile;
class WAL;
class WALReader;
class SeqReadFile;

class FileManager {
 public:
  static auto Exists(string_view path) -> bool;
  static auto IsDirectory(string_view path) -> bool;
  static auto Create(string_view path, FileOptions options) -> RC;
  static auto Destroy(string_view path) -> RC;
  static auto FixDirName(string_view path) -> string;
  static auto FixFileName(string_view path) -> string;
  static auto GetFileSize(string_view path, size_t *size) -> RC;
  static auto ReName(string_view old_path, string_view new_path) -> RC;
  static auto HandleHomeDir(string_view path) -> string;
  /* open */
  static auto OpenWritAbleFile(string_view filename, WritAbleFile **result) -> RC;
  static auto OpenTempFile(string_view dir_path, string_view subfix, TempFile **result) -> RC;
  static auto OpenAppendOnlyFile(string_view filename, WritAbleFile **result) -> RC;

  static auto OpenSeqReadFile(string_view filename, SeqReadFile **result) -> RC;

  static auto OpenMmapReadAbleFile(string_view file_name, MmapReadAbleFile **result) -> RC;
  static auto OpenRandomAccessFile(string_view filename, RandomAccessFile **result) -> RC;
  static auto ReadFileToString(string_view filename, string &result) -> RC;

  static auto OpenWAL(string_view dbname, int64_t log_number, WAL **result) -> RC;
  static auto OpenWALReader(string_view dbname, int64_t log_number, WALReader **result) -> RC;
  static auto OpenWALReader(string_view wal_file_path, WALReader **result) -> RC;

  static auto ReadDir(string_view directory_path, vector<string> &result) -> RC;
  static auto ReadDir(string_view directory_path, const std::function<bool(string_view)> &filter,
                      const std::function<void(string_view)> &handle_result) -> RC;
};

/* 缓冲顺序写 */
class WritAbleFile {
 public:
  WritAbleFile(string_view file_path, int fd);
  WritAbleFile(const WritAbleFile &)                     = delete;
  auto operator=(const WritAbleFile &) -> WritAbleFile & = delete;
  virtual ~WritAbleFile();
  auto Append(string_view data) -> RC;
  auto Close() -> RC;
  auto Flush() -> RC;
  auto Sync() -> RC;
  auto ReName(string_view new_file) -> RC;

  auto        GetPath() -> string;
  static auto Open(string_view file_path, WritAbleFile **result) -> RC;

 public:
  static constexpr size_t K_WRIT_ABLE_FILE_BUFFER_SIZE = 1 << 16;  // 64KB

 protected:
  std::string file_path_;                         // 文件路径
  int         fd_;                                // 文件描述符
  bool        closed_;                            // 是否关闭
  size_t      pos_;                               // 当前写入位置
  char        buf_[K_WRIT_ABLE_FILE_BUFFER_SIZE]; /* buffer */
};

/* 顺序读 */
class SeqReadFile {
 public:
  SeqReadFile(string_view filename, int fd);
  ~SeqReadFile();
  auto Read(size_t len, string &buffer, string_view &result) -> RC;
  auto Skip(size_t n) -> RC;
  auto GetPath() -> string;
  auto Close() -> RC;

 private:
  string filename_;
  int    fd_;
  bool   closed_;
};

/* 临时写文件 */
class TempFile : public WritAbleFile {
 public:
  TempFile(const std::string &file_path, int fd);
  static auto Open(string_view dir_path, string_view subfix, TempFile **result) -> RC;
};

/* mmap 读 */
class MmapReadAbleFile {
 public:
  MmapReadAbleFile(string_view file_name, char *base_addr, size_t file_size);
  ~MmapReadAbleFile();
  auto Read(size_t offset, size_t len, string_view &buffer) -> RC;
  auto Size() { return file_size_; }
  auto GetFileName() -> string { return file_name_; }

 private:
  char  *base_addr_;  // mmap 地址
  size_t file_size_;  // 文件大小
  string file_name_;  // 文件名
};

/* 随机读 */
class RandomAccessFile {
 public:
  RandomAccessFile(string_view filename, int fd);
  auto Read(size_t offset, size_t len, string_view &buffer, bool use_extra_buffer = false) -> RC;
  ~RandomAccessFile();

 private:
  string file_name_;
  int    fd_;
};

/**
 * @brief 文件元数据
 */
struct FileMetaData {
  FileMetaData() = default;

  size_t        file_size_{};
  int           num_keys_{};
  int           belong_to_level_{};
  int64_t       max_seq_{};
  MemKey        max_inner_key_;
  MemKey        min_inner_key_;
  unsigned char sha256_[SHA256_DIGEST_LENGTH];

  /* 获取在 db 中的 file 路径 */
  auto GetSSTablePath(string_view dbname) -> string;
  auto GetOid() const -> string;
  auto operator<(const FileMetaData &f) -> bool { return min_inner_key_ < f.min_inner_key_; }
};

auto operator<<(ostream &os, const FileMetaData &meta) -> ostream &;

auto WriteN(int fd, const char *buf, size_t len) -> ssize_t;
auto ReadN(int fd, const char *buf, size_t len) -> ssize_t;

auto LevelDir(string_view dbname) -> string;
auto LevelDir(string_view dbname, int n) -> string;
auto LevelFile(string_view level_dir, string_view sha256_hex) -> string;
auto RevDir(string_view dbname) -> string;
auto RevFile(string_view rev_dir, string_view sha256_hex) -> string;
auto CurrentFile(string_view dbname) -> string;
auto SstDir(string_view dbname) -> string;
auto SstFile(string_view sst_dir, string_view sha256_hex) -> string;
auto WalDir(string_view dbname) -> string;
auto WalFile(string_view wal_dir, int64_t log_number) -> string;
auto ParseWalFile(string_view filename, int64_t &seq) -> RC;
auto RemoveDirectory(const char *path) -> int;

}  // namespace lsm_tree