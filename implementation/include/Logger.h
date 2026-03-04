#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <iostream>
#include <ctime>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <vector>
#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(dir) _mkdir(dir)
#else
    #include <sys/stat.h>
    #define MKDIR(dir) mkdir(dir, 0755)
#endif

// Helper: Create a directory if it doesn't exist.
inline void createDirectory(const std::string& dir) {
    MKDIR(dir.c_str());
}

// ========================= Implementation Logger =========================
class ImplementationLogger {
public:
    enum LogLevel {
        INFO,
        DEBUG,
        LOG_ERROR
    };

    // Initializes the logger.
    static void init(const std::string& logFilePath = "logs/implementation_log.txt") {
        std::lock_guard<std::mutex> guard(mutex_);
        createDirectory("logs");
        instance().logFile_.open(logFilePath, std::ios::app);
        if (!instance().logFile_.is_open()) {
            std::cerr << "Error: Could not open implementation log file: " << logFilePath << std::endl;
        }
    }

    // Logs a message.
    static void log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> guard(mutex_);
        if (!instance().logFile_.is_open()) {
            std::cerr << "Implementation log file not open!" << std::endl;
            return;
        }
        std::time_t now = std::time(nullptr);
        char buf[80];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %X", std::localtime(&now));

        std::string levelStr;
        switch (level) {
            case INFO: levelStr = "INFO"; break;
            case DEBUG: levelStr = "DEBUG"; break;
            case LOG_ERROR: levelStr = "ERROR"; break;
            default: levelStr = "INFO"; break;
        }
        instance().logFile_ << "[" << buf << "] " << levelStr << ": " << message << std::endl;
    }

    static void close() {
        std::lock_guard<std::mutex> guard(mutex_);
        if (instance().logFile_.is_open()) {
            instance().logFile_.close();
        }
    }

private:
    ImplementationLogger() = default;
    ~ImplementationLogger() = default;
    ImplementationLogger(const ImplementationLogger&) = delete;
    ImplementationLogger& operator=(const ImplementationLogger&) = delete;

    static ImplementationLogger& instance() {
        static ImplementationLogger loggerInstance;
        return loggerInstance;
    }

    std::ofstream logFile_;
    static std::mutex mutex_;
};

std::mutex ImplementationLogger::mutex_;

// ============================ Process Logger =============================
class ProcessLogger {
public:
    // Initializes the process logger.
    static void init(const std::string& logFilePath = "logs/process_log.txt") {
        std::lock_guard<std::mutex> guard(mutex_);
        createDirectory("logs");
        instance().logFile_.open(logFilePath, std::ios::app);
        if (!instance().logFile_.is_open()) {
            std::cerr << "Error: Could not open process log file: " << logFilePath << std::endl;
        }
    }

    // Updates the process log with a table-format output.
    static void update(const std::vector<struct Process>& processes) {
        std::ostringstream oss;
        int pidWidth = 7, nameWidth = 20, statusWidth = 15, priorityWidth = 8, memoryWidth = 12, ownerWidth = 15;
        
        // Lambda to center align a string.
        auto centerString = [](const std::string &s, int width) -> std::string {
            int len = s.length();
            if (width <= len) return s;
            int pad = width - len;
            int padLeft = pad / 2;
            int padRight = pad - padLeft;
            return std::string(padLeft, ' ') + s + std::string(padRight, ' ');
        };
        
        // Lambda to create a horizontal line.
        auto horizontalLine = [&]() -> std::string {
            return "+" + std::string(pidWidth, '-') + "+" + std::string(nameWidth, '-') +
                   "+" + std::string(statusWidth, '-') + "+" + std::string(priorityWidth, '-') + 
                   "+" + std::string(memoryWidth, '-') + "+" + std::string(ownerWidth, '-') + "+\n";
        };
        
        oss << horizontalLine();
        oss << "|" << centerString("PID", pidWidth)
            << "|" << centerString("Name", nameWidth)
            << "|" << centerString("Status", statusWidth)
            << "|" << centerString("Priority", priorityWidth)
            << "|" << centerString("Mem (MB)", memoryWidth)
            << "|" << centerString("Owner", ownerWidth)
            << "|\n";
        oss << horizontalLine();
        for(const auto &proc : processes) {
            std::string displayName = proc.name;
            if(displayName.length() > nameWidth) {
                displayName = displayName.substr(0, nameWidth - 3) + "...";
            }
            
            std::stringstream memStream;
            memStream << std::fixed << std::setprecision(2) << proc.memoryMB;

            oss << "|" << centerString(std::to_string(proc.pid), pidWidth)
                << "|" << centerString(displayName, nameWidth)
                << "|" << centerString(proc.status, statusWidth)
                << "|" << centerString(std::to_string(proc.priority), priorityWidth)
                << "|" << centerString(memStream.str(), memoryWidth)
                << "|" << centerString(proc.owner, ownerWidth)
                << "|\n";
        }
        oss << horizontalLine();
        
        logTable(oss.str());
    }
    
    // Logs the given table string.
    static void logTable(const std::string &tableStr) {
        std::lock_guard<std::mutex> guard(mutex_);
        if (!instance().logFile_.is_open()) {
            std::cerr << "Process log file not open!" << std::endl;
            return;
        }
        instance().logFile_ << tableStr << std::endl;
    }
    
    static void close() {
        std::lock_guard<std::mutex> guard(mutex_);
        if (instance().logFile_.is_open()) {
            instance().logFile_.close();
        }
    }
    
private:
    ProcessLogger() = default;
    ~ProcessLogger() = default;
    ProcessLogger(const ProcessLogger&) = delete;
    ProcessLogger& operator=(const ProcessLogger&) = delete;
    
    static ProcessLogger& instance() {
        static ProcessLogger loggerInstance;
        return loggerInstance;
    }
    
    std::ofstream logFile_;
    static std::mutex mutex_;
};

std::mutex ProcessLogger::mutex_;

#endif // LOGGER_H
