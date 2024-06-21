- [1. 文件管理器详细规范说明文档](#1-文件管理器详细规范说明文档)
  - [1.1. 概述](#11-概述)
  - [1.2. 枚举](#12-枚举)
    - [1.2.1. `enum class FileOptions`](#121-enum-class-fileoptions)
  - [1.3. 类](#13-类)
    - [1.3.1. `WritAbleFile`](#131-writablefile)
    - [1.3.2. `TempFile`](#132-tempfile)
    - [1.3.3. `MmapReadAbleFile`](#133-mmapreadablefile)
    - [1.3.4. `RandomAccessFile`](#134-randomaccessfile)
    - [1.3.5. `WAL`](#135-wal)
    - [1.3.6. `WALReader`](#136-walreader)
    - [1.3.7. `SeqReadFile`](#137-seqreadfile)
  - [1.4. 函数](#14-函数)
    - [1.4.1. 存在性检查](#141-存在性检查)
      - [1.4.1.1. `auto Exists(string_view path) -> bool`](#1411-auto-existsstring_view-path---bool)
      - [1.4.1.2. `auto IsDirectory(string_view path) -> bool`](#1412-auto-isdirectorystring_view-path---bool)
    - [1.4.2. 创建和销毁](#142-创建和销毁)
      - [1.4.2.1. `auto Create(string_view path, FileOptions options) -> RC`](#1421-auto-createstring_view-path-fileoptions-options---rc)
      - [1.4.2.2. `auto Destroy(string_view path) -> RC`](#1422-auto-destroystring_view-path---rc)
    - [1.4.3. 名称处理](#143-名称处理)
      - [1.4.3.1. `auto FixDirName(string_view path) -> string`](#1431-auto-fixdirnamestring_view-path---string)
      - [1.4.3.2. `auto FixFileName(string_view path) -> string`](#1432-auto-fixfilenamestring_view-path---string)
    - [1.4.4. 文件操作](#144-文件操作)
      - [1.4.4.1. `auto GetFileSize(string_view path, size_t *size) -> RC`](#1441-auto-getfilesizestring_view-path-size_t-size---rc)
      - [1.4.4.2. `auto ReName(string_view old_path, string_view new_path) -> RC`](#1442-auto-renamestring_view-old_path-string_view-new_path---rc)
      - [1.4.4.3. `auto HandleHomeDir(string_view path) -> string`](#1443-auto-handlehomedirstring_view-path---string)
    - [1.4.5. 打开文件](#145-打开文件)
      - [1.4.5.1. `auto OpenWritAbleFile(string_view filename, WritAbleFile **result) -> RC`](#1451-auto-openwritablefilestring_view-filename-writablefile-result---rc)
      - [1.4.5.2. `auto OpenTempFile(string_view dir_path, string_view subfix, TempFile **result) -> RC`](#1452-auto-opentempfilestring_view-dir_path-string_view-subfix-tempfile-result---rc)
      - [1.4.5.3. `auto OpenAppendOnlyFile(string_view filename, WritAbleFile **result) -> RC`](#1453-auto-openappendonlyfilestring_view-filename-writablefile-result---rc)
      - [1.4.5.4. `auto OpenSeqReadFile(string_view filename, SeqReadFile **result) -> RC`](#1454-auto-openseqreadfilestring_view-filename-seqreadfile-result---rc)
      - [1.4.5.5. `auto OpenMmapReadAbleFile(string_view file_name, MmapReadAbleFile **result) -> RC`](#1455-auto-openmmapreadablefilestring_view-file_name-mmapreadablefile-result---rc)
      - [1.4.5.6. `auto OpenRandomAccessFile(string_view filename, RandomAccessFile **result) -> RC`](#1456-auto-openrandomaccessfilestring_view-filename-randomaccessfile-result---rc)
      - [1.4.5.7. `auto ReadFileToString(string_view filename, string &result) -> RC`](#1457-auto-readfiletostringstring_view-filename-string-result---rc)
      - [1.4.5.8. `auto OpenWAL(string_view dbname, int64_t log_number, WAL **result) -> RC`](#1458-auto-openwalstring_view-dbname-int64_t-log_number-wal-result---rc)
      - [1.4.5.9. `auto OpenWALReader(string_view dbname, int64_t log_number, WALReader **result) -> RC`](#1459-auto-openwalreaderstring_view-dbname-int64_t-log_number-walreader-result---rc)
      - [1.4.5.10. `auto OpenWALReader(string_view wal_file_path, WALReader **result) -> RC`](#14510-auto-openwalreaderstring_view-wal_file_path-walreader-result---rc)
    - [1.4.6. 目录操作](#146-目录操作)
      - [1.4.6.1. `auto ReadDir(string_view directory_path, vector<string> &result) -> RC`](#1461-auto-readdirstring_view-directory_path-vectorstring-result---rc)
      - [1.4.6.2. `auto ReadDir(string_view directory_path, const std::function<bool(string_view)> &filter, const std::function<void(string_view)> &handle_result) -> RC`](#1462-auto-readdirstring_view-directory_path-const-stdfunctionboolstring_view-filter-const-stdfunctionvoidstring_view-handle_result---rc)

# 1. 文件管理器详细规范说明文档

## 1.1. 概述

`file_manager` 命名空间提供了一系列用于管理文件和目录的函数和类。它提供了各种操作，例如创建、销毁、读取、写入和操作文件和目录。这些函数返回一个结果代码 (`RC`)，用于指示操作的成功或失败。

## 1.2. 枚举

### 1.2.1. `enum class FileOptions`
定义了用于创建文件或目录的选项。

- `DIR_`: 表示路径是一个目录。
- `FILE_`: 表示路径是一个文件。

## 1.3. 类

### 1.3.1. `WritAbleFile`
代表一个可写文件的类。

### 1.3.2. `TempFile`
代表一个临时文件的类。

### 1.3.3. `MmapReadAbleFile`
代表一个可使用内存映射读取的文件的类。

### 1.3.4. `RandomAccessFile`
代表一个随机访问文件的类。

### 1.3.5. `WAL`
代表一个Write-Ahead Logging (WAL) 文件的类。

### 1.3.6. `WALReader`
代表一个用于读取WAL文件的类。

### 1.3.7. `SeqReadFile`
代表一个顺序读取文件的类。

## 1.4. 函数

### 1.4.1. 存在性检查

#### 1.4.1.1. `auto Exists(string_view path) -> bool`
检查指定路径的文件或目录是否存在。

- **参数**: `path` - 要检查的路径。
- **返回值**: 如果路径存在，返回 `true`；否则返回 `false`。

#### 1.4.1.2. `auto IsDirectory(string_view path) -> bool`
检查指定路径是否是目录。

- **参数**: `path` - 要检查的路径。
- **返回值**: 如果路径是目录，返回 `true`；否则返回 `false`。

### 1.4.2. 创建和销毁

#### 1.4.2.1. `auto Create(string_view path, FileOptions options) -> RC`
创建一个文件或目录。

- **参数**: 
  - `path` - 要创建的文件或目录的路径。
  - `options` - 文件选项（`DIR_` 或 `FILE_`）。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.2.2. `auto Destroy(string_view path) -> RC`
销毁指定路径的文件或目录。

- **参数**: `path` - 要销毁的文件或目录的路径。
- **返回值**: 操作的结果代码 (`RC`)。

### 1.4.3. 名称处理

#### 1.4.3.1. `auto FixDirName(string_view path) -> string`
修正目录名称，确保其以斜杠结尾。

- **参数**: `path` - 要修正的目录路径。
- **返回值**: 修正后的目录路径。

#### 1.4.3.2. `auto FixFileName(string_view path) -> string`
修正文件名称。

- **参数**: `path` - 要修正的文件路径。
- **返回值**: 修正后的文件路径。

### 1.4.4. 文件操作

#### 1.4.4.1. `auto GetFileSize(string_view path, size_t *size) -> RC`
获取文件大小。

- **参数**: 
  - `path` - 文件路径。
  - `size` - 用于存储文件大小的指针。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.4.2. `auto ReName(string_view old_path, string_view new_path) -> RC`
重命名文件或目录。

- **参数**: 
  - `old_path` - 原始路径。
  - `new_path` - 新路径。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.4.3. `auto HandleHomeDir(string_view path) -> string`
处理主目录路径。

- **参数**: `path` - 要处理的路径。
- **返回值**: 处理后的路径。

### 1.4.5. 打开文件

#### 1.4.5.1. `auto OpenWritAbleFile(string_view filename, WritAbleFile **result) -> RC`
打开一个可写文件。

- **参数**: 
  - `filename` - 文件名。
  - `result` - 用于存储结果文件指针的指针。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.5.2. `auto OpenTempFile(string_view dir_path, string_view subfix, TempFile **result) -> RC`
在指定目录中打开一个临时文件。

- **参数**: 
  - `dir_path` - 目录路径。
  - `subfix` - 文件后缀。
  - `result` - 用于存储结果文件指针的指针。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.5.3. `auto OpenAppendOnlyFile(string_view filename, WritAbleFile **result) -> RC`
打开一个只追加写入的文件。

- **参数**: 
  - `filename` - 文件名。
  - `result` - 用于存储结果文件指针的指针。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.5.4. `auto OpenSeqReadFile(string_view filename, SeqReadFile **result) -> RC`
打开一个顺序读取文件。

- **参数**: 
  - `filename` - 文件名。
  - `result` - 用于存储结果文件指针的指针。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.5.5. `auto OpenMmapReadAbleFile(string_view file_name, MmapReadAbleFile **result) -> RC`
打开一个可使用内存映射读取的文件。

- **参数**: 
  - `file_name` - 文件名。
  - `result` - 用于存储结果文件指针的指针。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.5.6. `auto OpenRandomAccessFile(string_view filename, RandomAccessFile **result) -> RC`
打开一个随机访问文件。

- **参数**: 
  - `filename` - 文件名。
  - `result` - 用于存储结果文件指针的指针。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.5.7. `auto ReadFileToString(string_view filename, string &result) -> RC`
读取文件内容到字符串。

- **参数**: 
  - `filename` - 文件名。
  - `result` - 用于存储文件内容的字符串。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.5.8. `auto OpenWAL(string_view dbname, int64_t log_number, WAL **result) -> RC`
打开一个WAL文件。

- **参数**: 
  - `dbname` - 数据库名。
  - `log_number` - 日志编号。
  - `result` - 用于存储结果WAL文件指针的指针。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.5.9. `auto OpenWALReader(string_view dbname, int64_t log_number, WALReader **result) -> RC`
打开一个用于读取WAL文件的读取器。

- **参数**: 
  - `dbname` - 数据库名。
  - `log_number` - 日志编号。
  - `result` - 用于存储结果WAL读取器指针的指针。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.5.10. `auto OpenWALReader(string_view wal_file_path, WALReader **result) -> RC`
通过文件路径打开一个用于读取WAL文件的读取器。

- **参数**: 
  - `wal_file_path` - WAL文件路径。
  - `result` - 用于存储结果WAL读取器指针的指针。
- **返回值**: 操作的结果代码 (`RC`)。

### 1.4.6. 目录操作

#### 1.4.6.1. `auto ReadDir(string_view directory_path, vector<string> &result) -> RC`
读取目录内容到字符串向量中。

- **参数**: 
  - `directory_path` - 目录路径。
  - `result` - 用于存储目录内容的字符串向量。
- **返回值**: 操作的结果代码 (`RC`)。

#### 1.4.6.2. `auto ReadDir(string_view directory_path, const std::function<bool(string_view)> &filter, const std::function<void(string_view)> &handle_result) -> RC`
读取目录内容，并使用过滤器和结果处理函数进行处理。

- **参数**: 
  - `directory_path` - 目录路径。
  - `filter` - 用于过滤目录内容的函数。
  - `handle_result` - 用于处理过滤后结果的函数。
- **返回值**: 操作的结果代码 (`RC`)。