#include "Log.hpp"
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "spdlog/sinks/rotating_file_sink.h"
#include "Common.hpp"
namespace SimpleNodeEditor
{
    
Log::Log()
{        
    std::vector<spdlog::sink_ptr> logSinks;
    logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    logSinks.emplace_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("SimpleNodeEditor.log", 1024*1024*10, 3));
    logSinks[0]->set_pattern("%^[%T.%e] %n: %v%$");
    logSinks[1]->set_pattern("[%T.%e] [%l] %n: %v");

    m_spdLogger = std::make_shared<spdlog::logger>("SNELogger", begin(logSinks), end(logSinks));
    spdlog::register_logger(m_spdLogger);

}

void Log::SetLogLevel(const std::string& logLevelStr)
{
    LogLevel logLevel = Log::LogLevel::LogInfo;
    if (logLevelStr == "debug")
    {
        logLevel = Log::LogLevel::LogDebug;
    }
    else if (logLevelStr == "error")
    {
        logLevel = Log::LogLevel::LogError;
    }
    else if (logLevelStr == "warn")
    {

        logLevel = Log::LogLevel::LogWarn;
    }
    else 
    {
        logLevel = Log::LogLevel::LogInfo;
    }
}

void Log::SetLogLevel(LogLevel loglevel)
{
    switch (loglevel)
    {
        case LogLevel::LogTrace:
            GetInstance().GetLogger()->set_level(spdlog::level::trace);
            break;
        case LogLevel::LogDebug:
            GetInstance().GetLogger()->set_level(spdlog::level::debug);
            break;
        case LogLevel::LogInfo:
            GetInstance().GetLogger()->set_level(spdlog::level::info);
            break;
        case LogLevel::LogWarn:
            GetInstance().GetLogger()->set_level(spdlog::level::warn);
            break;
        case LogLevel::LogError:
            GetInstance().GetLogger()->set_level(spdlog::level::err);
            break;
        default:
            GetInstance().GetLogger()->set_level(spdlog::level::info);
            break;
    }
}

std::shared_ptr<spdlog::logger>& Log::GetLogger()
{
    return m_spdLogger;
}

} // namespace SimpleNodeEditor

