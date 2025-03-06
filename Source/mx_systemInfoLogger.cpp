#include "mx_systemInfoLogger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Environment.h>
#include <mutex>

#ifdef _WIN32
    #include <windows.h>
    #include <intrin.h>
    #include <tchar.h>
#else
    #include <sys/sysinfo.h>
    #include <unistd.h>
    #include <cstdlib>
#endif

#define LOG_DIR "logs"
#define ARCHIVE_DIR "logs/archive/system_info.log"
#define LOG_FILE "logs/system_info.log"

// Singleton instance
SystemInfoLogger& SystemInfoLogger::getInstance() {
    static SystemInfoLogger instance;
    return instance;
}

// Private constructor
SystemInfoLogger::SystemInfoLogger() {
    Poco::File logDir(LOG_DIR);
    if (!logDir.exists()) logDir.createDirectories();
}

// Returns the current timestamp in proper format
std::string SystemInfoLogger::getCurrentTimestamp() {
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

// Returns the build timestamp
std::string SystemInfoLogger::getBuildTimestamp() {
    return std::string(__DATE__) + " " + std::string(__TIME__);
}

// Returns system uptime (boot time)
std::string SystemInfoLogger::getSystemUptime() {
    time_t bootTime;
    std::ostringstream oss;
    
    #ifdef _WIN32
        ULONGLONG uptime = GetTickCount64() / 1000;  // Convert milliseconds → seconds
        bootTime = time(nullptr) - uptime;
        struct tm* bt = localtime(&bootTime);
        oss << std::put_time(bt, "%Y-%m-%d %H:%M:%S");
        
        // Also add actual uptime in human-readable format
        DWORD uptimeDays = static_cast<DWORD>(uptime / (60 * 60 * 24));
        DWORD uptimeHours = static_cast<DWORD>((uptime % (60 * 60 * 24)) / (60 * 60));
        DWORD uptimeMinutes = static_cast<DWORD>((uptime % (60 * 60)) / 60);
        
        oss << " (Uptime: " << uptimeDays << " days, " << uptimeHours << " hours, " 
            << uptimeMinutes << " minutes)";
    #else
        struct sysinfo sysInfo;
        if (sysinfo(&sysInfo) == 0) {
            bootTime = time(nullptr) - sysInfo.uptime;
            struct tm* bt = localtime(&bootTime);
            oss << std::put_time(bt, "%Y-%m-%d %H:%M:%S");
            
            // Also add actual uptime in human-readable format
            long uptimeDays = sysInfo.uptime / (60 * 60 * 24);
            long uptimeHours = (sysInfo.uptime % (60 * 60 * 24)) / (60 * 60);
            long uptimeMinutes = (sysInfo.uptime % (60 * 60)) / 60;
            
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
    std::ostringstream gpuInfo;
#ifdef _WIN32
    // Get more detailed GPU information using WMI
    try {
        // First try using EnumDisplayDevices for basic info
        DISPLAY_DEVICE displayDevice;
        displayDevice.cb = sizeof(displayDevice);
        if (EnumDisplayDevices(nullptr, 0, &displayDevice, 0)) {
            gpuInfo << displayDevice.DeviceString;
            
            // Try to get video memory info
            DWORD dedicatedVideoMemory = 0;
            HMODULE hDXGI = LoadLibrary(_T("dxgi.dll"));
            if (hDXGI) {
                // We could add DXGI interface calls here to get more detailed GPU info
                // But this requires more complex setup with COM
                FreeLibrary(hDXGI);
            }
        } else {
            gpuInfo << "Unknown";
        }
    } catch (...) {
        gpuInfo << "Error retrieving GPU information";
    }
#else
    // Check if we're running in WSL
    bool isWSL = false;
    std::ifstream osRelease("/proc/sys/kernel/osrelease");
    std::string osReleaseContent;
    if (osRelease.is_open()) {
        std::getline(osRelease, osReleaseContent);
        isWSL = (osReleaseContent.find("Microsoft") != std::string::npos || 
                 osReleaseContent.find("WSL") != std::string::npos);
    }
    
    if (isWSL) {
        // In WSL, try to get GPU info from Windows using PowerShell
        FILE* pipe = popen("powershell.exe -Command \"Get-WmiObject Win32_VideoController | Select-Object -ExpandProperty Name\"", "r");
        if (pipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                gpuInfo << buffer;
                pclose(pipe);
                return gpuInfo.str();
            }
            pclose(pipe);
            
            // Alternative approach for WSL
            pipe = popen("powershell.exe -Command \"(Get-ItemProperty -Path 'HKLM:\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000' -Name HardwareInformation.qwMemorySize -ErrorAction SilentlyContinue).HardwareInformation.qwMemorySize\"", "r");
            if (pipe) {
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    gpuInfo << "Windows GPU with " << std::stoll(buffer) / (1024*1024) << " MB VRAM";
                    pclose(pipe);
                    return gpuInfo.str();
                }
                pclose(pipe);
            }
        }
        
        // Try another approach with DirectX Diagnostic Tool
        pipe = popen("powershell.exe -Command \"dxdiag /t dxdiag_output.txt; Start-Sleep -Seconds 2; Select-String -Path dxdiag_output.txt -Pattern 'Card name' | Select-Object -First 1\"", "r");
        if (pipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string result(buffer);
                size_t pos = result.find(":");
                if (pos != std::string::npos) {
                    gpuInfo << result.substr(pos + 1);
                    pclose(pipe);
                    return gpuInfo.str();
                }
            }
            pclose(pipe);
        }
        
        // If all else fails for WSL
        gpuInfo << "WSL environment (GPU details unavailable)";
        return gpuInfo.str();
    }
    
    // Standard Linux approaches (if not in WSL)
    // Check for NVIDIA GPU
    FILE* pipe = popen("nvidia-smi --query-gpu=name,memory.total --format=csv,noheader 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            gpuInfo << buffer;
        }
        pclose(pipe);
        
        // Return if we found NVIDIA GPU info
        if (gpuInfo.str().length() > 0) {
            return gpuInfo.str();
        }
    }

    // Check for AMD GPU
    std::ifstream amdFile("/sys/class/drm/card0/device/vendor");
    if (amdFile.is_open()) {
        std::string vendorID;
        std::getline(amdFile, vendorID);
        amdFile.close();

        if (vendorID == "0x1002") {
            std::ifstream amdNameFile("/sys/class/drm/card0/device/device");
            std::string deviceID;
            if (amdNameFile.is_open()) {
                std::getline(amdNameFile, deviceID);
                amdNameFile.close();
                gpuInfo << "AMD Device (ID: " << deviceID << ")";
                return gpuInfo.str();
            }
            gpuInfo << "AMD GPU Detected";
            return gpuInfo.str();
        }
    }

    // Try generic approach for integrated/other GPUs
    pipe = popen("lspci | grep -i 'vga\\|3d\\|display' 2>/dev/null", "r");
    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            gpuInfo << buffer;
        } else {
            gpuInfo << "Not Found";
        }
        pclose(pipe);
    } else {
        gpuInfo << "Not Found";
    }
#endif
    return gpuInfo.str();
}

// Collects system information
std::string SystemInfoLogger::getSystemInfo() {
    std::ostringstream info;
    try {
        // Section 1: System Information
        info << "===== System Information =====" << std::endl;
        info << "OS              : " << Poco::Environment::osName() << " "
             << Poco::Environment::osVersion() << " ("
             << Poco::Environment::osArchitecture() << ")" << std::endl;

        // Check if running in WSL
        bool isWSL = false;
        #ifndef _WIN32
        std::ifstream osRelease("/proc/sys/kernel/osrelease");
        std::string osReleaseContent;
        if (osRelease.is_open()) {
            std::getline(osRelease, osReleaseContent);
            isWSL = (osReleaseContent.find("Microsoft") != std::string::npos || 
                    osReleaseContent.find("WSL") != std::string::npos);
            if (isWSL) {
                info << "Environment     : Windows Subsystem for Linux (WSL)" << std::endl;
            }
        }
        #endif

        // Get CPU Name
        info << "CPU             : ";
#ifdef _WIN32
        char cpuName[49] = { 0 };
        int cpuInfo[4] = { 0 };
        __cpuid(cpuInfo, 0x80000002);
        memcpy(cpuName, cpuInfo, sizeof(cpuInfo));
        __cpuid(cpuInfo, 0x80000003);
        memcpy(cpuName + 16, cpuInfo, sizeof(cpuInfo));
        __cpuid(cpuInfo, 0x80000004);
        memcpy(cpuName + 32, cpuInfo, sizeof(cpuInfo));
        info << cpuName << std::endl;
#else
        std::ifstream cpuFile("/proc/cpuinfo");
        std::string line;
        bool cpuFound = false;
        while (std::getline(cpuFile, line)) {
            if (line.find("model name") != std::string::npos) {
                info << line.substr(line.find(":") + 2) << std::endl;
                cpuFound = true;
                break;
            }
        }
        if (!cpuFound) {
            info << "Unable to retrieve information" << std::endl;
        }
#endif

        info << "CPU Cores       : " << Poco::Environment::processorCount() << std::endl;

        // Get RAM information
        info << "RAM             : ";
#ifdef _WIN32
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus)) {
            double totalRamGB = static_cast<double>(memStatus.ullTotalPhys) / (1024 * 1024 * 1024);
            double availableRamGB = static_cast<double>(memStatus.ullAvailPhys) / (1024 * 1024 * 1024);
            info << std::fixed << std::setprecision(2) << totalRamGB << " GB (Available: " 
                 << availableRamGB << " GB)" << std::endl;
        } else {
            info << "Unable to retrieve information" << std::endl;
        }
#else
        struct sysinfo sysInfo;
        if (sysinfo(&sysInfo) == 0) {
            double totalRamGB = static_cast<double>(sysInfo.totalram * sysInfo.mem_unit) / (1024 * 1024 * 1024);
            double availableRamGB = static_cast<double>((sysInfo.freeram + sysInfo.bufferram) * sysInfo.mem_unit) / (1024 * 1024 * 1024);
            info << std::fixed << std::setprecision(2) << totalRamGB << " GB (Available: " 
                 << availableRamGB << " GB)" << std::endl;
        } else {
            info << "Unable to retrieve information" << std::endl;
        }
#endif

        // Get GPU information
        info << "GPU             : " << getGPUInfo() << std::endl;
        
        // Section 2: Process Time Information
        
        info << "System Boot Time               : " << getSystemUptime() << std::endl;
        info << "Process Start Current Time     : " << getCurrentTimestamp() << std::endl;
        info << "Build Timestamp                : " << getBuildTimestamp() << std::endl;
        
        // Section 3: Additional Information
        info << std::endl;
        info << "Hostname        : " << Poco::Environment::nodeName() << std::endl;
        info << "Username        : " << Poco::Environment::get("USERNAME", Poco::Environment::get("USER", "Unknown")) << std::endl;

    } catch (const std::exception& e) {
        info << "Error collecting system information: " << e.what() << std::endl;
    } catch (...) {
        info << "Unknown error collecting system information" << std::endl;
    }

    return info.str();
}

// Archives old logs
void SystemInfoLogger::archiveOldLogs() {
    try {
        Poco::File logFile(LOG_FILE);
        Poco::File archiveDir(ARCHIVE_DIR);
        if (!archiveDir.exists()) {
            archiveDir.createDirectories();
        }

        if (logFile.exists()) {
            Poco::DateTime now;
            std::string archivePath = ARCHIVE_DIR ;

            Poco::File oldArchive(archivePath);
            if (oldArchive.exists()) {
                oldArchive.remove();
            }

            logFile.renameTo(archivePath);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error archiving logs: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error archiving logs" << std::endl;
    }
}

// Writes system info to file
void SystemInfoLogger::writeSystemInfoToFile() {
    try {
        std::ofstream file(LOG_FILE);
        if (file.is_open()) {
            std::string info = getSystemInfo();
            file << info;
            file.close();
            std::cout << "System info written to " << LOG_FILE << std::endl;
        } else {
            std::cerr << "Failed to open log file for writing: " << LOG_FILE << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error writing system info: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error writing system info" << std::endl;
    }
}

// Public function to log system info
void SystemInfoLogger::logSystemInfo() 
{
    std::lock_guard<std::mutex> lock(mutex_); // Ensure thread safety
    archiveOldLogs(); // Archive previous logs
    writeSystemInfoToFile(); // Generate new log
}
