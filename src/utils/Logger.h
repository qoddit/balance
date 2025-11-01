#pragma once

#include <string>
#include <fstream>
#include <mutex>

namespace Balance {

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

/**
 * Thread-safe logging system
 */
class Logger {
public:
    static Logger& instance();

    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filePath);
    void enableConsoleOutput(bool enable);

    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);

private:
    Logger();
    ~Logger();

    LogLevel currentLevel;
    std::ofstream logFile;
    bool consoleOutput;
    std::mutex mutex;

    std::string levelToString(LogLevel level);
    std::string getCurrentTimestamp();
};

// Convenience macros
#define LOG_DEBUG(msg) Balance::Logger::instance().debug(msg)
#define LOG_INFO(msg) Balance::Logger::instance().info(msg)
#define LOG_WARNING(msg) Balance::Logger::instance().warning(msg)
#define LOG_ERROR(msg) Balance::Logger::instance().error(msg)
#define LOG_CRITICAL(msg) Balance::Logger::instance().critical(msg)

} // namespace Balance
