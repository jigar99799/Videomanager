#include "mx_logger.h"
#include <dlfcn.h>
#include <iostream>
#include <thread>
#include <chrono>

MxLogger::MxLogger() : m_hLogger(nullptr), m_loggerWrite(nullptr),
    m_loggerInit(nullptr), m_loggerShutdown(nullptr),
    m_loggerIsInitialized(nullptr) 
{
}

MxLogger& MxLogger::instance() 
{
    static MxLogger inst;
    return inst;
}

MXLOGGER_STATUS_CODE MxLogger::initialize(const char* configPath) 
{
    std::cerr << "Initializing MxLogger with config: " << configPath << std::endl;

    if (m_hLogger) 
    {
        std::cerr << "Logger already initialized" << std::endl;
        return MXLOGGER_STATUS_ALLREADY_INITIALIZED;
    }

    // Try to load the shared library with different paths
    const char* libraryPaths[] = 
    {
        "./libSpdlog.so",
        "./libSpdlog.so.1",
        "./libSpdlog.so.1.0",
        "/loggerdll/libSpdlog.so",
        "/loggerdll/libSpdlog.so.1",
        "/loggerdll/libSpdlog.so.1.0",
        "/usr/local/lib/libSpdlog.so",
        "/usr/local/lib/libSpdlog.so.1",
        "/usr/local/lib/libSpdlog.so.1.0"
    };

    for (const char* path : libraryPaths) 
    {
        std::cerr << "Trying to load library from: " << path << std::endl;
        m_hLogger = dlopen(path, RTLD_LAZY);
        if (m_hLogger) 
        {
            std::cerr << "Successfully loaded library from: " << path << std::endl;
            break;
        }
    }

    if (!m_hLogger) 
    {
        std::cerr << "Failed to load library. Last error: " << dlerror() << std::endl;
        std::cerr << "Tried the following paths:" << std::endl;
        for (const char* path : libraryPaths) 
        {
            std::cerr << "  " << path << std::endl;
        }
        return MXLOGGER_INIT_FAILED;
    }
  
    // Load all function pointers
    std::cerr << "Loading function pointers..." << std::endl;
    m_loggerInit = (MX_Logger_Initialize)dlsym(m_hLogger, "MX_Logger_Initialize");
    m_loggerWrite = (MX_Logger_WriteFunc)dlsym(m_hLogger, "MX_Logger_Write");
    m_loggerShutdown = (MX_Logger_ShutdownFunc)dlsym(m_hLogger, "MX_Logger_Shutdown");
    m_loggerIsInitialized = (MX_Logger_IsInitializedFunc)dlsym(m_hLogger, "MX_Logger_IsInitialized");

    if (!m_loggerInit || !m_loggerWrite || !m_loggerShutdown || !m_loggerIsInitialized) 
    {
        std::cerr << "Failed to load one or more function pointers:" << std::endl;
        std::cerr << "  m_loggerInit: " << (m_loggerInit ? "OK" : "FAILED") << std::endl;
        std::cerr << "  m_loggerWrite: " << (m_loggerWrite ? "OK" : "FAILED") << std::endl;
        std::cerr << "  m_loggerShutdown: " << (m_loggerShutdown ? "OK" : "FAILED") << std::endl;
        std::cerr << "  m_loggerIsInitialized: " << (m_loggerIsInitialized ? "OK" : "FAILED") << std::endl;
        shutdown();
        return MXLOGGER_INIT_FAILED;
    }
    std::cerr << "All function pointers loaded successfully" << std::endl;
   
    // Call shutdown first in case the logger was not properly shut down previously
    if (m_loggerShutdown) 
    {
        std::cerr << "Calling shutdown before initialization..." << std::endl;
        m_loggerShutdown();
    }

    std::cerr << "Calling MX_Logger_Initialize..." << std::endl;
    MXLOGGER_STATUS_CODE status = m_loggerInit(configPath);
    std::cerr << "MX_Logger_Initialize returned status: " << status << std::endl;
    return status;
}

void MxLogger::shutdown() 
{
    if (m_hLogger) 
    {
        if (m_loggerShutdown) 
        {
            // Add a small delay to allow pending messages to be processed
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            try 
            {

                m_loggerShutdown();
                // Add another small delay before closing the library
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                dlclose(m_hLogger);
                m_hLogger = nullptr;
                m_loggerWrite = nullptr;
                m_loggerInit = nullptr;
                m_loggerShutdown = nullptr;
                m_loggerIsInitialized = nullptr;
            }
            catch (...) 
            {
                std::cerr << "Exception during logger shutdown" << std::endl;
            }
        }
    }
}

void MxLogger::write(LOGLEVEL level, const char* module, const char* message,
                    const char* file, const char* function, int line) 
{
    static int messageCount = 0;
    messageCount++;
    
    if (m_loggerWrite) {
        m_loggerWrite(level, module, message, file, function, line);
    } 
    else 
    {
        std::cerr << "Error: Logger write function is not initialized" << std::endl;
    }
}

bool MxLogger::isValid() const 
{
    return m_hLogger != nullptr && m_loggerWrite != nullptr;
}

MxLogger::~MxLogger() 
{
    shutdown();
}
