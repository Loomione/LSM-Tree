#include "util/monitor_logger.hh"
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

namespace lsm_tree {

auto MonitorLogger::Logger() -> MonitorLogger & {
  static MonitorLogger instance;
  return instance;
}

/* 目前的语意 是 第一次调用成功，后面都失败 */
void MonitorLogger::SetDbNameAndOptions(const std::string &db_name, const DBOptions *options) {
  try {
    logger_ =
        spdlog::basic_logger_mt<spdlog::async_factory>(options->logger_name_, db_name + '/' + options->log_file_name_);
    // std::cout << db_name + '/' + options->log_file_name << std::endl;
    logger_->set_pattern(options->log_pattern_);
    logger_->set_level(options->log_level_);
    spdlog::flush_every(std::chrono::seconds(1));
    spdlog::flush_on(spdlog::level::info);
    spdlog::flush_on(spdlog::level::debug);
    // then spdloge::get("logger_name")
    // spdlog::register_logger(logger);
  } catch (std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    exit(-1);
  }
}

MonitorLogger::MonitorLogger() : logger_(spdlog::stdout_color_mt("console")) {}
}  // namespace lsm_tree