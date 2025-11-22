#ifndef LOG_H
#define LOG_H
#include "spdlog/spdlog.h"
#include <memory>

namespace SimpleNodeEditor
{
class Log
{
public:
    enum class LogLevel
    {
        LogTrace,
        LogDebug,
        LogInfo,
        LogWarn,
        LogError,
        Unkown
    };

public:
    static Log& GetInstance()
    {
        static Log s_logInstance;
        return s_logInstance;
    };
    void                             SetLogLevel(LogLevel logLevel);
    void                             SetLogLevel(const std::string& logLevelStr);
    std::shared_ptr<spdlog::logger>& GetLogger();

private:
    Log();
    Log(const Log&)            = delete;
    Log& operator=(const Log&) = delete;

    std::shared_ptr<spdlog::logger> m_spdLogger;
};
} // end namespace SimpleNodeEditor

#define SNELOG_TRACE(...) ::SimpleNodeEditor::Log::GetInstance().GetLogger()->trace(__VA_ARGS__)
#define SNELOG_DEBUG(...) ::SimpleNodeEditor::Log::GetInstance().GetLogger()->debug(__VA_ARGS__)
#define SNELOG_INFO(...)  ::SimpleNodeEditor::Log::GetInstance().GetLogger()->info(__VA_ARGS__)
#define SNELOG_WARN(...)  ::SimpleNodeEditor::Log::GetInstance().GetLogger()->warn(__VA_ARGS__)
#define SNELOG_ERROR(...) ::SimpleNodeEditor::Log::GetInstance().GetLogger()->error(__VA_ARGS__)
#define SNELOG_CRITICAL(...) \
    ::SimpleNodeEditor::Log::GetInstance().GetLogger()->critical(__VA_ARGS__)

#endif // LOG_H