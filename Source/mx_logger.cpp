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
    if (m_hLogger) 
    {
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
        /*"/usr/local/lib/libSpdlog.so",
        "/usr/local/lib/libSpdlog.so.1",
        "/usr/local/lib/libSpdlog.so.1.0"*/
    };

    for (const char* path : libraryPaths) 
    {
        m_hLogger = dlopen(path, RTLD_LAZY);
        if (m_hLogger) 
        {
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
    m_loggerInit = (MX_Logger_Initialize)dlsym(m_hLogger, "MX_Logger_Initialize");
    m_loggerWrite = (MX_Logger_WriteFunc)dlsym(m_hLogger, "MX_Logger_Write");
    m_loggerShutdown = (MX_Logger_ShutdownFunc)dlsym(m_hLogger, "MX_Logger_Shutdown");
    m_loggerIsInitialized = (MX_Logger_IsInitializedFunc)dlsym(m_hLogger, "MX_Logger_IsInitialized");

    if (!m_loggerInit || !m_loggerWrite || !m_loggerShutdown || !m_loggerIsInitialized) 
    {
        std::cerr << "Failed to load one or more function pointers:" << std::endl;
        shutdown();
        return MXLOGGER_INIT_FAILED;
    }
   
    // Call shutdown first in case the logger was not properly shut down previously
    if (m_loggerShutdown) 
    {
        m_loggerShutdown();
    }

    MXLOGGER_STATUS_CODE status = m_loggerInit(configPath);
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
                    const char* file, const char* function, int line,const char* threadId) 
{
    if (m_loggerWrite) 
    {
        m_loggerWrite(level, module, message, file, function, line,threadId);
    } 
    else 
    {
        std::cerr << "Error: Logger write function is not initialized" << std::endl;
    }
}

const char* MxLogger::getCurrentThreadId()
{
	static thread_local std::string tls_threadIdStr;
	std::stringstream ss;
	ss << std::this_thread::get_id();
	tls_threadIdStr = ss.str();

	return tls_threadIdStr.c_str();
}

bool MxLogger::isValid() const 
{
    return m_hLogger != nullptr && m_loggerWrite != nullptr;
}

MxLogger::~MxLogger() 
{
    shutdown();
}
