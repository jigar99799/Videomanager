#include "mx_systemInfoLogger.h"
#include <Poco/Environment.h>
#include <Poco/File.h>
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/Path.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <mutex>
#include <thread>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
    #include <intrin.h>
    #include <direct.h>  // For _mkdir
#else
    #include <sys/sysinfo.h>
    #include <sys/utsname.h>
    #include <unistd.h>
    #include <sys/stat.h>  // For mkdir
#endif

// Constants for logging
#define LOG_DIR "logs"
#define ARCHIVE_DIR "logs/archive/system_info.log"
#define LOG_FILE "logs/system_info.log"
#define ARCHIVE_FILE   "logs/archive/system_info.log"

// Singleton instance
SystemInfoLogger& SystemInfoLogger::getInstance() 
{
    static SystemInfoLogger instance;
    return instance;
}

// Private constructor
SystemInfoLogger::SystemInfoLogger() 
    : receiveQueueInitialized(false)
    , transmitQueueInitialized(false)
    , logHandlerInitialized(false)
    , allComponentsInitialized(false)
{
    Poco::File logDir(LOG_DIR);
    if (!logDir.exists()) 
        logDir.createDirectories();

    // Mark all components as initialized
    allComponentsInitialized = true;
}

// Helper function to get current timestamp
std::string getCurrentTimestamp() 
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
    
    return std::string(buffer);
}

// Returns the build timestamp
std::string SystemInfoLogger::getBuildTimestamp() 
{
    return std::string(__DATE__) + " " + std::string(__TIME__);
}

// Get system uptime information
std::string SystemInfoLogger::getSystemUptime() 
{
    std::ostringstream oss;
    
    #ifdef _WIN32
        ULONGLONG uptime = GetTickCount64() / 1000;  // Convert milliseconds → seconds
        time_t bootTime = time(nullptr) - uptime;
        struct tm* bt = localtime(&bootTime);
        oss << std::put_time(bt, "%Y-%m-%d %H:%M:%S");
        
        // Also add actual uptime in human-readable format
        DWORD uptimeDays = static_cast<DWORD>(uptime / (60 * 60 * 24));
        DWORD uptimeHours = static_cast<DWORD>((uptime % (60 * 60 * 24)) / (60 * 60));
        DWORD uptimeMinutes = static_cast<DWORD>((uptime % (60 * 60)) / 60);
        
        oss << " (Uptime: " << uptimeDays << " days, " << uptimeHours << " hours, " 
            << uptimeMinutes << " minutes)";
    #else
        // For Linux systems
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            time_t bootTime = time(nullptr) - si.uptime;
            struct tm* bt = localtime(&bootTime);
            oss << std::put_time(bt, "%Y-%m-%d %H:%M:%S");
            
            // Also add actual uptime in human-readable format
            long uptimeDays = si.uptime / (60 * 60 * 24);
            long uptimeHours = (si.uptime % (60 * 60 * 24)) / (60 * 60);
            long uptimeMinutes = (si.uptime % (60 * 60)) / 60;
            
            oss << " (Uptime: " << uptimeDays << " days, " << uptimeHours << " hours, " 
                << uptimeMinutes << " minutes)";
        } else {
            oss << "Unable to retrieve system uptime";
        }
    #endif

    return oss.str();
}

// Fetches GPU information
std::string SystemInfoLogger::getGPUInfo() 
{
    std::string gpuInfo = "Unknown";
    
    #ifdef _WIN32
        // Basic Windows implementation - in a real app, you'd use DXGI or similar
        // This is just a placeholder for the concept
        gpuInfo = "Intel(R) HD Graphics 630";
    #else
        // Basic Linux implementation - in a real app, you'd parse lspci or /proc
        // For simplicity, attempt to get basic GPU info
        std::ifstream gpuFile("/proc/device-tree/model");
        if (gpuFile.is_open()) {
            std::getline(gpuFile, gpuInfo);
            gpuFile.close();
        } else {
            // Try alternative method
            FILE* cmd = popen("lspci | grep -i vga", "r");
            if (cmd) {
                char buffer[256];
                if (fgets(buffer, sizeof(buffer), cmd) != nullptr) {
                    gpuInfo = buffer;
                    // Clean up the output a bit
                    size_t pos = gpuInfo.find("VGA compatible controller:");
                    if (pos != std::string::npos) {
                        gpuInfo = gpuInfo.substr(pos + 25); // Skip past the header
                    }
                    pos = gpuInfo.find('\n');
                    if (pos != std::string::npos) {
                        gpuInfo = gpuInfo.substr(0, pos);
                    }
                }
                pclose(cmd);
            }
            
            // Fallback if all else fails
            if (gpuInfo == "Unknown") {
                gpuInfo = "Intel(R) HD Graphics 630";
            }
        }
    #endif
    
    return gpuInfo;
}

// Collects system information
std::string SystemInfoLogger::getSystemInfo() 
{
    std::ostringstream info;
    
    try {
        info << "===== System Information =====" << std::endl;
        
        // OS Information
        std::string osName;
        std::string environment;
        
        #if defined(_WIN32)
            osName = "Windows";
            OSVERSIONINFOEX osvi;
            ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
            
            // Note: GetVersionEx is deprecated, but still works for basic info
            // In a production app, you'd use RtlGetVersion or VerifyVersionInfo
            GetVersionEx((OSVERSIONINFO*)&osvi);
            
            std::ostringstream osVersion;
            osVersion << osName << " " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion;
            osName = osVersion.str();
            
            environment = "Native Windows";
        #else
            // For Linux, get OS details from /etc/os-release if available
            std::ifstream osRelease("/etc/os-release");
            if (osRelease.is_open()) {
                std::string line;
                while (std::getline(osRelease, line)) {
                    if (line.find("PRETTY_NAME=") == 0) {
                        osName = line.substr(12);
                        // Remove quotes if present
                        if (osName.front() == '"' && osName.back() == '"') {
                            osName = osName.substr(1, osName.length() - 2);
                        }
                        break;
                    }
                }
                osRelease.close();
            }
            
            if (osName.empty()) {
                // Fallback to uname if /etc/os-release is not available
                struct utsname unameData;
                if (uname(&unameData) == 0) {
                    osName = std::string(unameData.sysname) + " " + unameData.release + " (" + unameData.machine + ")";
                } else {
                    osName = "Linux (Unknown Version)";
                }
            }
            
            // Check if running in WSL
            std::ifstream procVersion("/proc/version");
            if (procVersion.is_open()) {
                std::string versionInfo;
                std::getline(procVersion, versionInfo);
                procVersion.close();
                
                if (versionInfo.find("Microsoft") != std::string::npos || 
                    versionInfo.find("WSL") != std::string::npos) {
                    environment = "Windows Subsystem for Linux (WSL)";
                } else {
                    environment = "Native Linux";
                }
            } else {
                environment = "Native Linux";
            }
        #endif
        
        info << "OS              : " << osName << std::endl;
        if (!environment.empty()) {
            info << "Environment     : " << environment << std::endl;
        }
        
        // CPU Information
        std::string cpuModel;
        #if defined(_WIN32)
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                TCHAR value[1024];
                DWORD size = sizeof(value);
                DWORD type;
                if (RegQueryValueEx(hKey, "ProcessorNameString", NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
                    cpuModel = value;
                }
                RegCloseKey(hKey);
            }
        #else
            // For Linux, parse /proc/cpuinfo
            std::ifstream cpuinfo("/proc/cpuinfo");
            if (cpuinfo.is_open()) {
                std::string line;
                while (std::getline(cpuinfo, line)) {
                #if defined(__arm__) || defined(__aarch64__)
                    // For ARM CPUs, look for "model name" or "Processor"
                    if (line.find("model name") == 0 || line.find("Processor") == 0) {
                        size_t pos = line.find(':');
                        if (pos != std::string::npos) {
                            cpuModel = line.substr(pos + 2);
                            break;
                        }
                    }
                #else
                    // For x86/x64 CPUs, look for "model name"
                    if (line.find("model name") == 0) {
                        size_t pos = line.find(':');
                        if (pos != std::string::npos) {
                            cpuModel = line.substr(pos + 2);
                            break;
                        }
                    }
                #endif
                }
                cpuinfo.close();
            }
        #endif
        
        if (cpuModel.empty()) {
            cpuModel = "Unknown";
        }
        
        info << "CPU             : " << cpuModel << std::endl;
        
        // CPU Cores
        int cores = Poco::Environment::processorCount();
        info << "CPU Cores       : " << cores << std::endl;
        
        // RAM Information
        #if defined(_WIN32)
            MEMORYSTATUSEX memInfo;
            memInfo.dwLength = sizeof(MEMORYSTATUSEX);
            if (GlobalMemoryStatusEx(&memInfo)) {
                double totalRAM = memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
                double availableRAM = memInfo.ullAvailPhys / (1024.0 * 1024.0 * 1024.0);
                
                info << "RAM             : " << std::fixed << std::setprecision(2) << totalRAM << " GB ";
                info << "(Available: " << std::fixed << std::setprecision(2) << availableRAM << " GB)" << std::endl;
            }
        #else
            // For Linux, parse /proc/meminfo
            std::ifstream meminfo("/proc/meminfo");
            if (meminfo.is_open()) {
                std::string line;
                unsigned long totalRAM = 0, availableRAM = 0;
                
                while (std::getline(meminfo, line)) {
                    if (line.find("MemTotal:") == 0) {
                        sscanf(line.c_str(), "MemTotal: %lu", &totalRAM);
                    } else if (line.find("MemAvailable:") == 0) {
                        sscanf(line.c_str(), "MemAvailable: %lu", &availableRAM);
                    }
                }
                
                meminfo.close();
                
                if (totalRAM > 0) {
                    double totalRAM_GB = totalRAM / (1024.0 * 1024.0);
                    double availableRAM_GB = availableRAM / (1024.0 * 1024.0);
                    
                    info << "RAM             : " << std::fixed << std::setprecision(2) << totalRAM_GB << " GB ";
                    if (availableRAM > 0) {
                        info << "(Available: " << std::fixed << std::setprecision(2) << availableRAM_GB << " GB)";
                    }
                    info << std::endl;
                }
            }
        #endif
        
        // GPU Information
        info << "GPU             : " << getGPUInfo() << std::endl;
        
        // System Boot Time and Process Start Time
        info << std::endl;
        info << "System Boot Time               : " << getSystemUptime() << std::endl;
        info << "Process Start Current Time     : " << getCurrentTimestamp() << std::endl;
        info << "Build Timestamp                : " << getBuildTimestamp() << std::endl;
        
        // Host and User info
        std::string hostname = Poco::Environment::nodeName();
        
        info << std::endl;
        info << "Hostname        : " << hostname << std::endl;
       
    }
    catch (const std::exception& e) {
        info << "Error collecting system information: " << e.what() << std::endl;
    }
    catch (...) {
        info << "Unknown error collecting system information" << std::endl;
    }
    
    return info.str();
}








// Writes system info to file
void SystemInfoLogger::writeSystemInfoToFile() 
{
    try {

        archiveOldLogs();

        // Get all information first to avoid multiple writes
        std::string systemInfo = getSystemInfo();
        std::string initializationInfo = getInitializationInfo();

      

        // Make sure directory exists
        #ifdef _WIN32
            if (_mkdir(LOG_DIR) != 0 && errno != EEXIST) {
                std::cerr << "Failed to create log directory: " << LOG_DIR << std::endl;
            }
        #else
            if (mkdir(LOG_DIR, 0777) != 0 && errno != EEXIST) {
                std::cerr << "Failed to create log directory: " << LOG_DIR << std::endl;
            }
        #endif

        // Write everything in a single operation
        std::ofstream file(LOG_FILE);
        if (file.is_open()) 
        {
            file << systemInfo;
            
            if (!initializationInfo.empty()) 
            {
                file << initializationInfo;
            }
            
            file << std::endl;
            file.close();
        }
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error writing system info to file: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error writing system info to file" << std::endl;
    }
}

// Generate initialization information as a string
std::string SystemInfoLogger::getInitializationInfo() {
    std::ostringstream info;
    
    try {

        // Only add initialization section if we have data to show
        if (!componentStatus.empty() || !libraryInfo.empty()) {
            info << std::endl << "===== Pipeline Configuration =====" << std::endl;
            
            // Write library information
            if (!libraryInfo.empty()) {
                info << std::endl << "Modules:" << std::endl;
                for (const auto& lib : libraryInfo) {
                    info << "  • " << lib.name << " v" << lib.version;
                    if (!lib.buildDate.empty()) {
                        info << " (Built: " << lib.buildDate << ")";
                    }
                    info << std::endl;
                }
            }
            
            // Write queue status in a dedicated section
            info << std::endl << "Communication Queues:" << std::endl;
            info << "  • Receive Queue : " << (receiveQueueInitialized ? "Online" : "Offline") << std::endl;
            info << "  • Transmit Queue: " << (transmitQueueInitialized ? "Online" : "Offline") << std::endl;
            
            // Write log handler status
            info << std::endl << "System Services:" << std::endl;
            info << "  • Log Handler   : " << (logHandlerInitialized ? "Active" : "Inactive") << std::endl;
            
            // Write component status in a better organized format
            if (!componentStatus.empty()) {
                info << "  • Components    : " << componentStatus.size() << " registered" << std::endl;
                
                // Count initialized components
                int initializedCount = 0;
                for (const auto& comp : componentStatus) {
                    if (comp.second.initialized) {
                        initializedCount++;
                    }
                }
                
                info << "    - " << initializedCount << " active, " 
                     << (componentStatus.size() - initializedCount) << " inactive" << std::endl;
            }
            
            // Write final status with more professional wording
            info << std::endl << "Pipeline Status: " 
                 << (areAllComponentsInitialized() ? "✓ Fully Operational" : "⚠ Partially Operational") 
                 << std::endl;
                 
            // Add initialization timestamp
            info << "Initialization Complete: " << getCurrentTimestamp() << std::endl;
        }
    } catch (const std::exception& e) {
        info << "Error generating initialization info: " << e.what() << std::endl;
    }
    
    return info.str();
}

// Check and update all components status
void SystemInfoLogger::checkAndUpdateAllComponentsStatus() 
{
    allComponentsInitialized = true;
    
    // Check each component
    for (const auto& comp : componentStatus) 
    {
        std::cerr << " comp name :  " << comp.first <<  " comp isinit :  " << comp.second.initialized << std::endl;

        if (!comp.second.initialized) 
        {
            allComponentsInitialized = false;
            break;
        }
    }
}

// Write component status to file
void SystemInfoLogger::writeComponentStatusToFile() {
    try {
        std::cerr << "Writing component status to file..." << std::endl;
        
        // Create log directory if it doesn't exist
        #ifdef _WIN32
            if (_mkdir(LOG_DIR) != 0 && errno != EEXIST) {
                std::cerr << "Failed to create log directory: " << LOG_DIR << " (Error: " << errno << ")" << std::endl;
                return;
            }
        #else
            if (mkdir(LOG_DIR, 0777) != 0 && errno != EEXIST) {
                std::cerr << "Failed to create log directory: " << LOG_DIR << " (Error: " << errno << ")" << std::endl;
                return;
            }
        #endif
        
        // Open file for writing (append mode)
        std::ofstream file(LOG_FILE, std::ios::app);
        if (file.is_open()) {
            std::cerr << "Log file opened for writing: " << LOG_FILE << std::endl;
            
            file << "===== Component Status Update at " << getCurrentTimestamp() << " =====" << std::endl;
            
            // Write status of all components
            for (const auto& comp : componentStatus) {
                file << comp.first << ": " 
                     << (comp.second.initialized ? "Initialized" : "Not Initialized") 
                     << " - " << comp.second.statusMessage 
                     << " (" << comp.second.timestamp << ")" << std::endl;
            }
            
            file << "Overall Status: " << (allComponentsInitialized ? "All Components Initialized" : "Not All Components Initialized") << std::endl;
            file << std::endl;
            
            file.close();
            
            std::cerr << "Component status written to file successfully" << std::endl;
        } else {
            std::cerr << "Failed to open log file for writing: " << LOG_FILE << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error writing component status to file: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error writing component status to file" << std::endl;
    }
}

// Initialize the receive queue
void SystemInfoLogger::initializeReceiveQueue(bool success, const std::string& message) {
    
    
    receiveQueueInitialized = success;
    updateComponentStatus("Receive Queue", success, message);
    
    // Check and update overall status
    checkAndUpdateAllComponentsStatus();
}

// Initialize the transmit queue
void SystemInfoLogger::initializeTransmitQueue(bool success, const std::string& message) {
  
    
    transmitQueueInitialized = success;
    updateComponentStatus("Transmit Queue", success, message);
    
    // Check and update overall status
    checkAndUpdateAllComponentsStatus();
}

// Initialize the log handler
void SystemInfoLogger::initializeLogHandler(bool success, const std::string& message) 
{
    
    logHandlerInitialized = success;
    updateComponentStatus("Log Handler", success, message);
    
    // Check and update overall status
    checkAndUpdateAllComponentsStatus();
}

// Register a new component for status tracking
void SystemInfoLogger::registerComponent(const std::string& componentName) 
{
    // Only register if not already present
    if (componentStatus.find(componentName) == componentStatus.end()) {
        ComponentStatus status;
        status.initialized = false;
        status.statusMessage = "Registered";
        status.timestamp = getCurrentTimestamp();
        componentStatus[componentName] = status;
    }
    
    // No longer write to file on every registration
}

// Update the status of a component
void SystemInfoLogger::updateComponentStatus(const std::string& componentName, bool success, const std::string& message) 
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Register if not already present
    if (componentStatus.find(componentName) == componentStatus.end()) {
        registerComponent(componentName);
    }
    
    // Update status
    ComponentStatus& status = componentStatus[componentName];
    status.initialized = success;
    status.statusMessage = message;
    status.timestamp = getCurrentTimestamp();
    
    // Write component status to file
    writeComponentStatusToFile();
    
    // Check if all components initialized
    checkAndUpdateAllComponentsStatus();
}

void SystemInfoLogger::archiveOldLogs() {
    try {
        Poco::File logFile(LOG_FILE);
        Poco::File archiveDir(ARCHIVE_DIR);
        if (!archiveDir.exists()) {
            archiveDir.createDirectories();
        }

        if (logFile.exists()) {
            Poco::DateTime now;
            std::string archivePath = ARCHIVE_DIR;

            Poco::File oldArchive(archivePath);
            if (oldArchive.exists()) {
                oldArchive.remove();
            }

            logFile.renameTo(archivePath);
        }
        }
    catch (const std::exception& e) {
        std::cerr << "Error archiving logs: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown error archiving logs" << std::endl;
    }
    }

// Public function to log system info
void SystemInfoLogger::logSystemInfo() 
{
    std::lock_guard<std::mutex> lock(mutex_); // Ensure thread safety
    
    std::cerr << "Logging system information..." << std::endl;
    
    try {
        // Create log directory if it doesn't exist
        #ifdef _WIN32
            if (_mkdir(LOG_DIR) != 0 && errno != EEXIST) {
                std::cerr << "Failed to create log directory: " << LOG_DIR << " (Error: " << errno << ")" << std::endl;
            }
        #else
            if (mkdir(LOG_DIR, 0777) != 0 && errno != EEXIST) {
                std::cerr << "Failed to create log directory: " << LOG_DIR << " (Error: " << errno << ")" << std::endl;
            }
        #endif
        
        // Archive old logs only if the file exists
        std::ifstream checkFile(LOG_FILE);
        if (checkFile.good()) {
            checkFile.close();
            archiveOldLogs(); // Archive previous logs
        } else {
            std::cerr << "No existing log file to archive" << std::endl;
        }
        
        // Get system information content
        std::string systemInfo = getSystemInfo();
        std::string initializationInfo = getInitializationInfo();
        
        // Write the new log file
        std::ofstream logFile(LOG_FILE);
        if (logFile.is_open()) {
            logFile << "===== System Information Log =====" << std::endl;
            logFile << "Generated on: " << getCurrentTimestamp() << std::endl;
            logFile << systemInfo;
            logFile << initializationInfo;
            logFile.close();
            
            std::cerr << "System information successfully written to: " << LOG_FILE << std::endl;
        } else {
            std::cerr << "Failed to open log file for writing: " << LOG_FILE << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in logSystemInfo: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error in logSystemInfo" << std::endl;
    }
}

// Check if all components are initialized
bool SystemInfoLogger::areAllComponentsInitialized() 
{
    // Use the pre-computed flag that is updated by checkAndUpdateAllComponentsStatus
    return allComponentsInitialized;
}

// Generate acknowledgment for Video Manager
std::string SystemInfoLogger::generateVideoManagerAcknowledgment() {
    std::ostringstream ack;
    
    ack << "===== Video Manager Pipeline Status =====" << std::endl;
    ack << "Timestamp: " << getCurrentTimestamp() << std::endl;
    ack << "Status: " << (areAllComponentsInitialized() ? "Ready" : "Not Ready") << std::endl;
    
    // Add component details
    ack << "Components:" << std::endl;
    for (const auto& comp : componentStatus) {
        ack << "  " << comp.first << ": " 
            << (comp.second.initialized ? "Ready" : "Failed") << std::endl;
    }
    
    return ack.str();
}

// Send acknowledgment to Video Manager
void SystemInfoLogger::sendAcknowledgmentToVideoManager() 
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Only send if all components are initialized
    if (areAllComponentsInitialized()) {
        std::string acknowledgment = generateVideoManagerAcknowledgment();
        
        // TODO: Implement the actual sending mechanism to Video Manager
        // This would typically involve some IPC, network call, etc.
        std::cout << "Sending acknowledgment to Video Manager:" << std::endl;
        std::cout << acknowledgment << std::endl;
    } else {
        std::cerr << "Cannot send acknowledgment - not all components are initialized" << std::endl;
    }
}

// Add library information
void SystemInfoLogger::addLibraryInfo(const std::string& name, const std::string& version, const std::string& buildDate) 
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    LibraryInfo info;
    info.name = name;
    info.version = version;
    info.buildDate = buildDate;
    
    libraryInfo.push_back(info);
    // No longer write to file on every update
}

// Check if a component is initialized
bool SystemInfoLogger::isComponentInitialized(const std::string& componentName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = componentStatus.find(componentName);
    if (it != componentStatus.end()) {
        return it->second.initialized;
    }
    
    return false;
}
