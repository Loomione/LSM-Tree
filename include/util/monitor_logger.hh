#pragma once
#include <memory>
#include <string>
#include "options.hh"
#include "spdlog/spdlog.h"

namespace lsm_tree {

class MonitorLogger {
 public:
  /* 默认 */
  MonitorLogger();
  static auto Logger() -> MonitorLogger &;

  void SetDbNameAndOptions(const std::string &db_name, const DBOptions *options);

  std::shared_ptr<spdlog::logger> logger_;
  // atomic<bool> default_log_;
};

#define MLogger MonitorLogger::Logger()
#define MLog MonitorLogger::Logger().logger_
}  // namespace lsm_tree
