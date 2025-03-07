#pragma once

#include <string>
#include <mutex>
#include <vector>
#include <map>

class SystemInfoLogger {
public:
    // Get singleton instance
    static SystemInfoLogger& getInstance();
    
    // Delete copy constructor and assignment operator
    SystemInfoLogger(const SystemInfoLogger&) = delete;
    SystemInfoLogger& operator=(const SystemInfoLogger&) = delete;
    
    // Public methods
    void logSystemInfo(); 
    
    // Component registration and status tracking
    void registerComponent(const std::string& componentName);
    void updateComponentStatus(const std::string& componentName, bool success, const std::string& message);
    bool isComponentInitialized(const std::string& componentName);
    
    // Queue and handler initialization
    void initializeReceiveQueue(bool success, const std::string& message);
    void initializeTransmitQueue(bool success, const std::string& message);
    void initializeLogHandler(bool success, const std::string& message);
    
    // Library and hardware information
    void addLibraryInfo(const std::string& name, const std::string& version, const std::string& buildDate);
    
    // Status and acknowledgment
    bool areAllComponentsInitialized();
    std::string generateVideoManagerAcknowledgment();
    void sendAcknowledgmentToVideoManager();
    
private:
    // Private constructor and destructor
    SystemInfoLogger();
    ~SystemInfoLogger() = default;
    
    // Private helper methods
    std::string getSystemUptime();
    std::string getSystemInfo();
    std::string getInitializationInfo();
    std::string getGPUInfo() ;
    std::string getBuildTimestamp() ;
    void archiveOldLogs();
    void writeSystemInfoToFile();
    void writeComponentStatusToFile();
    void checkAndUpdateAllComponentsStatus();
    
    // Component status structure
    struct ComponentStatus {
        bool initialized;
        std::string statusMessage;
        std::string timestamp;
    };
    
    // Library information structure
    struct LibraryInfo {
        std::string name;
        std::string version;
        std::string buildDate;
    };
    
    // Private members
    std::mutex mutex_;
    std::map<std::string, ComponentStatus> componentStatus;
    std::vector<LibraryInfo> libraryInfo;
    bool receiveQueueInitialized;
    bool transmitQueueInitialized;
    bool logHandlerInitialized;
    bool allComponentsInitialized;
};
