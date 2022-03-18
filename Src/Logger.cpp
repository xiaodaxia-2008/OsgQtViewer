#include "Logger.h"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace __inernal__
{
std::unique_ptr<spdlog::logger>
CreateLogger(const std::string &filepath = "Viewer.log")
{
    // init logger
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        filepath, 1048576 * 50, 3); // single file max 50MB
    file_sink->set_level(spdlog::level::trace);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    // don't change the sink sequence
    spdlog::sinks_init_list sinks = {file_sink, console_sink};
    auto logger = std::make_unique<spdlog::logger>("Viewer", sinks);
    logger->set_level(spdlog::level::trace);
    return logger;
}

void LoggerSetConsoleSinkLevel(spdlog::level::level_enum level)
{
    auto &sinks = logger->sinks();
    sinks.at(1)->set_level(level);
}

void LoggerSetFileSinkLevel(spdlog::level::level_enum level)
{
    auto &sinks = logger->sinks();
    sinks.at(0)->set_level(level);
}

std::unique_ptr<spdlog::logger> logger = CreateLogger();
}