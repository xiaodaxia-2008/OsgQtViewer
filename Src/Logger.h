// Copyright (c) RVBUST, Inc - All rights reserved.
#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#define LEVEL_TRACE spdlog::level::trace
#define LEVEL_DEBUG spdlog::level::debug
#define LEVEL_INFO spdlog::level::info
#define LEVEL_ERROR spdlog::level::error
#define LEVEL_CRITICAL spdlog::level::critical

#define LOG_TRACE(...)                                                         \
    __inernal__::logger->trace("{}:{}\n{}: {}", __FILE__, __LINE__,            \
                               __FUNCTION__, fmt::format(__VA_ARGS__));
#define LOG_DEBUG(...)                                                         \
    __inernal__::logger->debug("{}:{}\n{}: {}", __FILE__, __LINE__,            \
                               __FUNCTION__, fmt::format(__VA_ARGS__));
#define LOG_INFO(...)                                                          \
    __inernal__::logger->info("{}:{}\n{}: {}", __FILE__, __LINE__,             \
                              __FUNCTION__, fmt::format(__VA_ARGS__));

#define LOG_WARN(...)                                                          \
    __inernal__::logger->warn("{}:{}\n{}: {}", __FILE__, __LINE__,             \
                              __FUNCTION__, fmt::format(__VA_ARGS__));
#define LOG_ERROR(...)                                                         \
    __inernal__::logger->error("{}:{}\n{}: {}", __FILE__, __LINE__,            \
                               __FUNCTION__, fmt::format(__VA_ARGS__));
#define LOG_CRITICAL(...)                                                      \
    __inernal__::logger->critical("{}:{}\n{}: {}", __FILE__, __LINE__,         \
                                  __FUNCTION__, fmt::format(__VA_ARGS__));
#define LOG_SET_CONSOLE_SINK_LEVEL(level)                                      \
    __inernal__::LoggerSetConsoleSinkLevel(level);
#define LOG_SET_FILE_SINK_LEVEL(level)                                         \
    __internal__::LoggerSetFileSinkLevel(level);
#define LOG_SET_LEVEL(level) __inernal__::logger->set_level(level);

namespace __inernal__
{
extern std::unique_ptr<spdlog::logger> logger;
void LoggerSetConsoleSinkLevel(spdlog::level::level_enum level);
void LoggerSetFileSinkLevel(spdlog::level::level_enum level);
} // namespace __inernal__