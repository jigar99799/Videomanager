#pragma once

#include <string>
#include "App_enum.h"
#include <sstream>

// Function pointer types
typedef MXLOGGER_STATUS_CODE (*MX_Logger_Initialize)(const char* jsonConfigfilepath);
typedef void (*MX_Logger_WriteFunc)(LOGLEVEL level, const char* module, const char* message, const char* file, const char* function, int line,const char* threadId);
typedef void (*MX_Logger_ShutdownFunc)(void);
typedef bool (*MX_Logger_IsInitializedFunc)(void);
typedef void (*MX_Logger_FlushFunc)(void);

class MxLogger 
{
private:
    void* m_hLogger;  // Handle to the shared library
    MX_Logger_WriteFunc m_loggerWrite;
    MX_Logger_Initialize m_loggerInit;
    MX_Logger_ShutdownFunc m_loggerShutdown;
    MX_Logger_IsInitializedFunc m_loggerIsInitialized;
    MX_Logger_FlushFunc m_loggerFlush;

    MxLogger();  // Private constructor for singleton

public:
    static MxLogger& instance();
    
    // Delete copy constructor and assignment
    MxLogger(const MxLogger&) = delete;
    MxLogger& operator=(const MxLogger&) = delete;

    MXLOGGER_STATUS_CODE initialize(const char* configPath);
    void shutdown();
    void flush();
    void write(LOGLEVEL level, const char* module, const char* message,
               const char* file, const char* function, int line,const char* threadId);
    static const char* getCurrentThreadId();         
    bool isValid() const;
    ~MxLogger();
};

// Logging macros
#define MX_LOG_TRACE(module, message)    MxLogger::instance().write(Trace, module, message, __FILE__, __FUNCTION__, __LINE__, MxLogger::getCurrentThreadId())
#define MX_LOG_DEBUG(module, message)    MxLogger::instance().write(Debug, module, message, __FILE__, __FUNCTION__, __LINE__, MxLogger::getCurrentThreadId())
#define MX_LOG_INFO(module, message)     MxLogger::instance().write(Info, module, message, __FILE__, __FUNCTION__, __LINE__, MxLogger::getCurrentThreadId())
#define MX_LOG_WARN(module, message)     MxLogger::instance().write(Warning, module, message, __FILE__, __FUNCTION__, __LINE__, MxLogger::getCurrentThreadId())
#define MX_LOG_ERROR(module, message)    MxLogger::instance().write(Error, module, message, __FILE__, __FUNCTION__, __LINE__, MxLogger::getCurrentThreadId())
#define MX_LOG_CRITICAL(module, message) MxLogger::instance().write(Critical, module, message, __FILE__, __FUNCTION__, __LINE__, MxLogger::getCurrentThreadId())
