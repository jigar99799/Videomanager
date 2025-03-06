#ifndef SYSTEM_INFO_LOGGER_H
#define SYSTEM_INFO_LOGGER_H

#include <string>
#include <mutex>

class SystemInfoLogger {
public:
    static SystemInfoLogger& getInstance();  // Singleton access

    void logSystemInfo();  // Function to log system info

private:
    std::mutex mutex_; // For thread safety

    // Private Constructor (Singleton)
    SystemInfoLogger();
    
    // Disable copy & assignment
    SystemInfoLogger(const SystemInfoLogger&) = delete;
    SystemInfoLogger& operator=(const SystemInfoLogger&) = delete;

    // Internal helper functions
    std::string getCurrentTimestamp();
    std::string getBuildTimestamp();
    std::string getSystemUptime();
    std::string getSystemInfo();
    std::string getGPUInfo();
    void archiveOldLogs();
    void writeSystemInfoToFile();
};

#endif // SYSTEM_INFO_LOGGER_H
