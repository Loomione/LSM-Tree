#include "util/file_util.hh"
#include <fcntl.h>
#include <fmt/ostream.h>
#include <linux/limits.h>
#include <pwd.h>
#include <sys/dir.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include "util/hash_util.hh"
#include "util/monitor_logger.hh"
#include "wal.hh"
namespace lsm_tree {

/*
**********************************************************************************************************************************************
*  FileManager
**********************************************************************************************************************************************
*/

/**
 * @brief 判断文件是否存在
 *
 * @param path
 * @return true
 * @return false
 */
auto FileManager::Exists(string_view path) -> bool {
  struct stat buffer;
  return stat(path.data(), &buffer) == 0;
}

auto FileManager::IsDirectory(string_view path) -> bool {
  struct stat buffer;
  if (stat(path.data(), &buffer) == 0) {
    return S_ISDIR(buffer.st_mode);
  }
  return false;
}

/**
 * @brief 在指定路径创建文件或目录。
 * @param path 要创建的文件或目录的路径。
 * @param options 创建文件或目录的选项。
 * @return 表示操作成功或失败的结果代码。
 */
auto FileManager::Create(string_view path, FileOptions options) -> RC {
  switch (options) {
    case FileOptions::DIR_:
      if (auto err = mkdir(path.data(), 0755); err) {
        return RC::CREATE_DIRECTORY_FAILED;
      }
      break;
    case FileOptions::FILE_:
      if (auto err = open(path.data(), O_CREAT | O_RDWR, 0644); err < 0) {
        return RC::CREATE_FILE_FAILED;
      }
      break;
  }
  return RC::OK;
}

/**
 * @brief 处理给定路径中的主目录。
 *
 * 此函数接收一个路径，并检查它是否以波浪号（~）字符开头。
 * 如果是，则将波浪号替换为实际的主目录路径。
 * 如果不是以波浪号开头，则返回原始路径不变。
 *
 * @param path 要处理的输入路径。
 * @return 如果适用，则用主目录替换后的修改后的路径。
 */
auto FileManager::HandleHomeDir(string_view path) -> string {
  string true_path;

  if (!path.starts_with("~")) {
    true_path = path;
  } else {
    const char *homedir;
    if ((homedir = getenv("HOME")) == nullptr) {
      homedir = getpwuid(getuid())->pw_dir;
    }
    if (homedir != nullptr) {
      true_path = homedir;
      true_path += path.substr(1);
    }
  }
  return true_path;
}

/**
 * @brief 销毁一个目录。
 *
 * 此函数从文件系统中删除指定的目录。
 *
 * @param path 要销毁的目录的路径。
 * @return 如果成功销毁目录，则返回 RC::OK，否则返回 RC::DESTROY_DIRECTORY_FAILED。
 */
auto FileManager::Destroy(string_view path) -> RC {
  path = HandleHomeDir(path);
  if (IsDirectory(path)) {
    if (auto err = RemoveDirectory(path.data()); err) {
      return RC::DESTROY_DIRECTORY_FAILED;
    }
  } else {
    if (auto err = unlink(path.data()); err) {
      return RC::DESTROY_FILE_FAILED;
    }
  }
  return RC::OK;
}

auto FileManager::FixFileName(string_view path) -> string {
  if (path.starts_with("~")) {
    return HandleHomeDir(path);
  }
  return string(path);
}

/**
 * @brief 通过确保以斜杠结尾来修复目录名称。
 *
 * 如果给定的路径为空，则函数返回一个单独的点（'.'）表示当前目录。
 * 如果给定的路径不以波浪号（'~'）开头，则函数调用 HandleHomeDir 处理主目录。
 * 否则，函数使用给定的路径。
 * 最后，如果修复后的路径尚未以斜杠结尾，则函数会追加一个斜杠。
 *
 * @param path 要修复的目录路径。
 * @return 带有斜杠结尾的修复后的目录路径。
 */
auto FileManager::FixDirName(string_view path) -> string {
  if (path.empty()) {
    return ".";
  }
  string fix_path;
  if (path.starts_with("~")) {
    fix_path = HandleHomeDir(path);
  } else {
    fix_path = path;
  }
  if (fix_path.back() != '/') {
    fix_path += "/";
  }
  return fix_path;
}

/**
 * @brief 获取指定文件的大小。
 *
 * 此函数使用 `stat` 系统调用获取指定文件的文件状态信息，并从中提取文件大小。
 * 如果 `stat` 调用失败，则函数会记录错误信息并返回一个错误码，同时将 `size` 设置为 0。
 *
 * @param path 文件路径，类型为 `string_view`，表示要获取大小的文件。
 * @param size 引用类型参数，用于存储获取到的文件大小。
 * @return RC 返回操作的结果状态：
 *         - `RC::OK` 表示成功获取文件大小。
 *         - `RC::STAT_FILE_ERROR` 表示获取文件状态信息失败。
 */
auto FileManager::GetFileSize(string_view path, size_t &size) -> RC {
  struct stat file_stat;
  if (stat(path.data(), &file_stat) != 0) {
    MLog->error("can not get {} stat, error: {}", path.data(), strerror(errno));
    size = 0;
    return RC::STAT_FILE_ERROR;
  }
  size = file_stat.st_size;
  return RC::OK;
}

auto FileManager::OpenWritAbleFile(string_view filename, std::unique_ptr<WritAbleFile> &result) -> RC {
  return WritAbleFile::Open(filename, result);
}

auto FileManager::OpenTempFile(string_view dir_path, string_view subfix, std::unique_ptr<TempFile> &result) -> RC {
  return TempFile::Open(dir_path, subfix, result);
}

auto FileManager::OpenAppendOnlyFile(string_view filename, std::unique_ptr<WritAbleFile> &result) -> RC {
  int fd = ::open(filename.data(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
  if (fd < 0) {
    result = nullptr;
    return RC::OPEN_FILE_ERROR;
  }
  result.reset(new WritAbleFile(filename, fd));
  return RC::OK;
}

auto FileManager::OpenSeqReadFile(string_view filename, std::unique_ptr<SeqReadFile> &result) -> RC {
  int fd = ::open(filename.data(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    result = nullptr;
    return RC::OPEN_FILE_ERROR;
  }
  result.reset(new SeqReadFile(filename, fd));
  return RC::OK;
}

auto FileManager::OpenMmapReadAbleFile(string_view file_name, MmapReadAbleFile **result) -> RC {
  RC  rc = RC::OK;
  int fd = ::open(file_name.data(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    *result = nullptr;
    return RC::OPEN_FILE_ERROR;
  }
  size_t file_size = 0;
  if (rc = GetFileSize(file_name, file_size); rc != RC::OK) {
    // MLog->error("Failed to open mmap file {}, error: {}", file_name, strrc(rc));
    return rc;
  }
  if (auto base_addr = mmap(nullptr, file_size, PROT_READ, MAP_SHARED, fd, 0); base_addr == MAP_FAILED) {
    //  MLog->error("Failed to mmap file {}, error: {}", file_name, strerror(errno));
    rc = RC::MMAP_ERROR;
  } else {
    *result = new MmapReadAbleFile(file_name, static_cast<char *>(base_addr), file_size);
    rc      = RC::OK;
  }
  if (close(fd) != 0) {
    // MLog->error("Failed to close mmap file {}, error: {}", file_name, strerror(errno));
    rc = RC::CLOSE_FILE_ERROR;
  }
  return rc;
}

auto FileManager::ReadFileToString(string_view filename, string &result) -> RC {
  MmapReadAbleFile *mmap_readable_file{nullptr};
  RC                rc{RC::OK};
  if (rc = OpenMmapReadAbleFile(filename, &mmap_readable_file); rc != RC::OK) {
    // MLog->error("ReadFileToString {} error: {}", filename, strrc(rc));
  } else {
    auto file_size = mmap_readable_file->Size();
    result.resize(file_size);
    string_view result_view;
    if (auto rc = mmap_readable_file->Read(0, file_size, result_view); rc != RC::OK) {
      // MLog->error("mmap_readable_file {} Read error: {}", filename, strrc(rc));
      result = result_view;
    }
  }
  delete mmap_readable_file;
  return rc;
}

auto FileManager::OpenWAL(string_view dbname, int64_t log_number, WAL **result) -> RC {
  /* open append only file for wal */
  string                        wal_file_name = WalFile(WalDir(dbname), log_number);
  std::unique_ptr<WritAbleFile> wal_file;

  if (auto rc = OpenAppendOnlyFile(wal_file_name, wal_file); rc != RC::OK) {
    // MLog->error("open wal file {} failed: {} {}", wal_file_name, strrc(rc), strerror(errno));
    return rc;
  }
  *result = new WAL(wal_file);
  // MLog->info("wal_file {} created", wal_file_name);
  return RC::OK;
}

auto FileManager::OpenWALReader(string_view dbname, int64_t log_number, std::unique_ptr<WALReader> &result) -> RC {
  /* open append only file for wal */
  return OpenWALReader(WalFile(WalDir(dbname), log_number), result);
}

auto FileManager::OpenWALReader(string_view wal_file_path, std::unique_ptr<WALReader> &result) -> RC {
  /* open append only file for wal */
  std::unique_ptr<SeqReadFile> seq_read_file;
  if (auto rc = OpenSeqReadFile(wal_file_path, seq_read_file); rc != RC::OK) {
    return rc;
  }
  result.reset(new WALReader(seq_read_file));
  return RC::OK;
}

/* 获得一个文件夹的所有子文件 opendir+readdir+closedir */
auto FileManager::ReadDir(string_view directory_path, std::vector<std::string> &result) -> RC {
  result.clear();
  DIR *dir = opendir(directory_path.data());
  if (dir == nullptr) {
    return RC::IO_ERROR;
  }
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    result.emplace_back(entry->d_name);
  }
  closedir(dir);
  return RC::OK;
}

auto FileManager::ReadDir(string_view directory_path, const std::function<bool(string_view)> &filter,
                          const std::function<void(string_view)> &handle_result) -> RC {
  DIR *dir = opendir(directory_path.data());
  if (dir == nullptr) {
    return RC::IO_ERROR;
  }
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (filter(entry->d_name)) {
      handle_result(entry->d_name);
    }
  }
  closedir(dir);
  return RC::OK;
}

/*
**********************************************************************************************************************************************
* WritAbleFile
**********************************************************************************************************************************************
*/
WritAbleFile::WritAbleFile(string_view file_path, int fd) : file_path_(file_path), fd_(fd), pos_(0), closed_(false) {}

/**
 * @brief 使用指定的文件路径打开一个WritAbleFile。
 *
 * @param file_path 要打开的文件的路径。
 * @param result 指向将保存创建的 WritableFile 对象的指针的指针。
 * @return 如果成功打开文件，则返回 RC::OK，否则返回 RC::OPEN_FILE_ERROR。
 */
auto WritAbleFile::Open(string_view file_path, std::unique_ptr<WritAbleFile> &result) -> RC {
  // int fd = ::open(file_path.data(), O_WRONLY | O_CREAT | O_CLOEXEC | O_APPEND, 0644);  // 追加写文件
  int fd = ::open(file_path.data(), O_TRUNC | O_WRONLY | O_CREAT | O_CLOEXEC, 0644);  // 文件内容将被清空再打开
  if (fd < 0) {
    result.reset();
    return RC::OPEN_FILE_ERROR;
  }

  result.reset(new WritAbleFile(file_path, fd));
  return RC::OK;
}

auto WritAbleFile::Flush() -> RC {
  if (pos_ > 0) {
    if (auto written = WriteN(fd_, buf_, pos_); written == -1) {
      return RC::IO_ERROR;
    }
    pos_ = 0;
  }
  return RC::OK;
}

auto WritAbleFile::Close() -> RC {
  if (pos_ > 0) {
    if (auto rc = Flush(); rc == RC::OK) {
      return rc;
    }
  }
  if (fd_ != -1) {
    if (int ret = close(fd_); ret < 0) {
      return RC::CLOSE_FILE_ERROR;
    }
  }
  closed_ = true;
  return RC::OK;
}

auto WritAbleFile::Sync() -> RC {
  if (auto rc = Flush(); rc == RC::IO_ERROR) {
    return rc;
  }
  if (auto ret = fdatasync(fd_); ret == -1) {
    return RC::IO_ERROR;
  }
  return RC::OK;
}

/**
 * 将给定数据追加到可写文件中。
 *
 * @param data 要追加的数据。
 * @return 表示操作成功或失败的结果代码。
 */
auto WritAbleFile::Append(string_view data) -> RC {
  auto data_len = data.size();

  /* 没有超过限制 */
  if (pos_ + data_len < K_WRIT_ABLE_FILE_BUFFER_SIZE) {
    memcpy(buf_ + pos_, data.data(), data.size());
    pos_ += data_len;
    return RC::OK;
  }

  /* 超过限制则 1. 刷盘 2. 拷贝 | 直接写 */
  auto written = K_WRIT_ABLE_FILE_BUFFER_SIZE - pos_;
  memcpy(buf_ + pos_, data.data(), written);
  pos_ += written;
  auto left_data_len = data_len - written;
  if (auto rc = Flush(); rc != RC::OK) {
    return rc;
  }

  if (left_data_len > K_WRIT_ABLE_FILE_BUFFER_SIZE) {
    if (written = WriteN(fd_, data.data() + written, left_data_len); written != left_data_len) {
      return RC::IO_ERROR;
    }
  } else {
    memcpy(buf_, data.data() + written, left_data_len);
    pos_ += left_data_len;
  }

  return RC::OK;
}

auto WritAbleFile::ReName(string_view new_file) -> RC {
  string true_path = FileManager::FixFileName(new_file);
  if (auto ret = rename(file_path_.c_str(), true_path.c_str()); ret != 0) {
    MLog->error("tempfile name:{} rename to {} failed: {}", file_path_, true_path, strerror(errno));
    return RC::RENAME_FILE_ERROR;
  }
  file_path_ = std::move(true_path);
  // MLog->info("tempfile name:{} rename to {}", file_path_, true_path);
  return RC::OK;
}

auto WritAbleFile::GetPath() -> std::string { return file_path_; }

WritAbleFile::~WritAbleFile() {
  if (!closed_) {
    Close();
  }
}

/*
**********************************************************************************************************************************************
* SeqReadFile
**********************************************************************************************************************************************
*/

SeqReadFile::SeqReadFile(string_view filename, int fd) : filename_(filename), fd_(fd), closed_(false) {}
SeqReadFile::~SeqReadFile() { Close(); }

/**
 * @brief 从文件中读取数据到提供的缓冲区中。
 *
 * @param len 要从文件中读取的字节数。
 * @param buffer 用于存储读取数据的缓冲区。
 * @param result 将更新为读取数据的 string_view 的引用。
 * @return 表示读取操作状态的返回码。
 */
auto SeqReadFile::Read(size_t len, string &buffer, string_view &result) -> RC {
  RC rc = RC::OK;
  if (len > buffer.size()) {
    buffer.resize(len);
  }
  auto ret = ReadN(fd_, buffer.data(), len);
  if (ret == -1) {
    MLog->error("read_n failed");
    return RC::IO_ERROR;
  }
  result = {buffer.data(), static_cast<size_t>(ret)};
  return rc;
}

auto SeqReadFile::Skip(size_t n) -> RC {
  if (lseek(fd_, n, SEEK_CUR) == -1) {
    MLog->error("lseek failed");
    return RC::IO_ERROR;
  }
  return RC::OK;
}

auto SeqReadFile::GetPath() -> string { return filename_; }

auto SeqReadFile::Close() -> RC {
  if (!closed_) {
    if (auto ret = close(fd_); ret == -1) {
      return RC::CLOSE_FILE_ERROR;
    }
    closed_ = true;
    fd_     = -1;
  }
  return RC::OK;
}

/*
**********************************************************************************************************************************************
* TempFile
**********************************************************************************************************************************************
*/
TempFile::TempFile(const std::string &file_path, int fd) : WritAbleFile(file_path, fd) {}

/**
 * @brief 使用给定的目录路径和后缀打开一个临时文件。
 * @param dir_path 将创建临时文件的目录路径。
 * @param suffix 要添加到临时文件名的后缀。
 * @param result 指向将设置为新创建的 TempFile 对象的 TempFile 指针的指针。
 * @return RC 表示操作成功或失败的返回码。
 */
auto TempFile::Open(string_view dir_path, string_view subfix, std::unique_ptr<TempFile> &result) -> RC {
  string tmp_file(FileManager::FixDirName(dir_path));
  tmp_file += subfix;
  tmp_file += "XXXXXX";

  int fd = mkstemp(tmp_file.data());
  if (fd == -1) {
    result.reset();
    MLog->error("mkstemp {} failed: {}", tmp_file, strerror(errno));
    return RC::MAKESTEMP_ERROR;
  }

  std::string file_path =
      std::filesystem::read_symlink(std::filesystem::path("/proc/self/fd/") / std::to_string(fd)).string();
  // MLog->info("create new tempfile: {}", file_path);
  result.reset(new TempFile(file_path, fd));
  return RC::OK;
}

/*
**********************************************************************************************************************************************
* MmapReadAbleFile
**********************************************************************************************************************************************
*/
MmapReadAbleFile::MmapReadAbleFile(string_view file_name, char *base_addr, size_t file_size)
    : file_name_(file_name), base_addr_(base_addr), file_size_(file_size) {}

MmapReadAbleFile::~MmapReadAbleFile() {
  if (int ret = ::munmap(reinterpret_cast<void *>(base_addr_), file_size_); ret) {
    MLog->error("Failed to munmap file {}, error: {}", file_name_, strerror(errno));
  }
}

auto MmapReadAbleFile::Read(size_t offset, size_t len, string_view &buffer) -> RC {
  if (offset + len > file_size_) {
    buffer = {};
    return RC::OUT_OF_RANGE;
  }
  buffer = {base_addr_ + offset, len};
  return RC::OK;
}

/*
**********************************************************************************************************************************************
* RandomAccessFile
**********************************************************************************************************************************************
*/
RandomAccessFile::RandomAccessFile(string_view filename, int fd) : file_name_(filename), fd_(fd) {}

auto RandomAccessFile::Read(size_t offset, size_t len, string_view &buffer, bool use_extra_buffer) -> RC {
  char *buf;
  if (!use_extra_buffer) {
    buf = new char[len];
  } else {
    buf = const_cast<char *>(buffer.data());
  }
  /* better than seek + read ; which will not change file pointer */
  ssize_t read_len = pread(fd_, buf, len, static_cast<off_t>(offset));
  if (read_len < 0) {
    if (!use_extra_buffer) {
      delete[] buf;
    }
    return RC::IO_ERROR;
  }
  if (!use_extra_buffer) {
    buffer = {buf, static_cast<size_t>(read_len)};
  }
  return RC::OK;
}

RandomAccessFile::~RandomAccessFile() {
  if (fd_ != -1) {
    close(fd_);
  }
}
/*
**********************************************************************************************************************************************
* FileMetaData
**********************************************************************************************************************************************
*/

/**
 * @brief 返回给定数据库名称的 SSTable 文件路径。
 *
 * @param dbname 数据库的名称。
 * @return SSTable 文件的路径。
 */
auto FileMetaData::GetSSTablePath(string_view dbname) -> string {
  return SstFile(SstDir(dbname), Sha256DigitToHex(sha256_));
}

/**
 * 返回文件的对象 ID（OID）。
 *
 * @return 文件的对象 ID（OID），以字符串形式返回。
 */
auto FileMetaData::GetOid() const -> string { return Sha256DigitToHex(sha256_); }

/**
 * 重载的流插入运算符，用于将 FileMetaData 对象输出到输出流中。
 *
 * @param os 要写入 FileMetaData 对象的输出流。
 * @param meta 要写入输出流的 FileMetaData 对象。
 * @return 写入 FileMetaData 对象后的输出流。
 */
auto operator<<(ostream &os, const FileMetaData &meta) -> std::ostream & {
  os << fmt::format(
      "@FileMetaData[ file_size={}, num_keys={}, max_seq={}, belong_to_level={}, "
      "max_inner_key={} "
      "min_inner_key={}, sha256={} ]\n",
      meta.file_size_, meta.num_keys_, meta.max_seq_, meta.belong_to_level_, meta.max_inner_key_, meta.min_inner_key_,
      Sha256DigitToHex(meta.sha256_));
  return os;
}

auto WriteN(int fd, const char *buf, size_t len) -> ssize_t {
  ssize_t n = 0;
  while (n < len) {
    ssize_t r = write(fd, buf + n, len - n);
    if (r < 0) {
      if (errno == EINTR) {
        continue;  // 如果被信号中断，重试写入
      }
      return -1;  // 发生其他错误，返回 -1
    }
    n += r;
  }
  return n;
}

auto ReadN(int fd, const char *buf, size_t len) -> ssize_t {
  ssize_t n = 0;
  while (n < len) {
    ssize_t r = read(fd, const_cast<void *>(static_cast<const void *>(buf + n)), len - n);
    if (r < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (r == 0) {
      return n;
    }
    n += r;
  }
  return n;
}

/**
 * @brief 递归地删除一个目录及其所有内容。
 *
 * 此函数删除指定的目录及其所有内容，包括子目录和文件。
 *
 * @param path 要删除的目录的路径。
 * @return 如果成功删除目录，则返回 0，否则返回 -1。
 */
auto RemoveDirectory(const char *path) -> int {
  DIR   *d        = opendir(path);
  size_t path_len = strlen(path);
  int    r        = -1;

  if (d != nullptr) {
    struct dirent *p;

    r = 0;
    while ((r == 0) && ((p = readdir(d)) != nullptr)) {
      int    r2 = -1;
      char  *buf;
      size_t len;

      /* Skip the names "." and ".." and "/" as we
       * don't want to recurse on them. */
      if ((strcmp(p->d_name, ".") == 0) || (strcmp(p->d_name, "..") == 0) || (strcmp(p->d_name, "/") == 0)) {
        continue;
      }

      len = path_len + strlen(p->d_name) + 2;
      buf = new char[len];

      if (buf != nullptr) {
        struct stat statbuf;

        snprintf(buf, len, "%s/%s", path, p->d_name);
        if (stat(buf, &statbuf) == 0) {
          if (S_ISDIR(statbuf.st_mode)) {
            r2 = RemoveDirectory(buf);
          } else {
            r2 = unlink(buf);
          }
        }
        delete[] buf;
      }
      r = r2;
    }
    closedir(d);
  }

  if (r == 0) {
    r = rmdir(path);
  }

  return r;
}

auto LevelDir(string_view dbname) -> string {
  string level_dir(dbname);
  if (!dbname.ends_with("/")) {
    level_dir += '/';
  }
  level_dir += "level/";
  return level_dir;
}

auto LevelDir(string_view dbname, int n) -> string {
  string level_dir(dbname);
  if (!dbname.ends_with("/")) {
    level_dir += '/';
  }
  level_dir += "level/" + std::to_string(n) + "/";
  return level_dir;
}

/**
 * Generates the file path for a level file based on the given level directory and SHA256 hash.
 *
 * @param level_dir The directory where the level file should be stored.
 * @param sha256_hex The SHA256 hash in hexadecimal format.
 * @return The file path for the level file.
 */
auto LevelFile(string_view level_dir, string_view sha256_hex) -> string {
  string file_path(level_dir);
  file_path += sha256_hex;
  file_path += ".lvl";
  return file_path;
}

/**
 * Returns the reverse directory path for the given database name.
 * If the database name does not end with a forward slash ('/'), it is appended.
 * The "rev/" directory is then appended to the database name.
 *
 * @param dbname The name of the database.
 * @return The reverse directory path.
 */
auto RevDir(string_view dbname) -> string {
  string rev_dir(dbname);
  if (!dbname.ends_with("/")) {
    rev_dir += '/';
  }
  rev_dir += "rev/";
  return rev_dir;
}

auto RevFile(string_view rev_dir, string_view sha256_hex) -> string {
  string file_path(rev_dir);
  file_path += sha256_hex;
  file_path += ".rev";
  return file_path;
}

auto CurrentFile(string_view dbname) -> string {
  string file_path(dbname);
  if (!dbname.ends_with("/")) {
    file_path += '/';
  }
  file_path += "CURRENT";
  return file_path;
}

auto SstDir(string_view dbname) -> string {
  string level_dir(dbname);
  if (!dbname.ends_with("/")) {
    level_dir += '/';
  }
  level_dir += "sst/";
  return level_dir;
}

auto SstFile(string_view sst_dir, string_view sha256_hex) -> string {
  return fmt::format("{}{}.sst", sst_dir, sha256_hex);
}

auto WalDir(string_view dbname) -> string {
  string wal_dir(dbname);
  if (!dbname.ends_with("/")) {
    wal_dir += '/';
  }
  wal_dir += "wal/";
  return wal_dir;
}

auto WalFile(string_view wal_dir, int64_t log_number) -> string { return fmt::format("{}{}.wal", wal_dir, log_number); }

auto ParseWalFile(string_view filename, int64_t &seq) -> RC {
  std::istringstream iss(string(filename.substr(0, filename.length() - strlen(".wal"))));
  iss >> seq;
  return RC::OK;
}

}  // namespace lsm_tree
