#include "PipelineProcess.h"
#include "mx_systemInfoLogger.h"
#include <string>
#include <string.h>
#include <Poco/Environment.h>

// Define static members
std::unique_ptr<PipelineProcess, std::function<void(PipelineProcess*)>> PipelineProcess::s_instance = 
    std::unique_ptr<PipelineProcess, std::function<void(PipelineProcess*)>>(
        nullptr, 
        [](PipelineProcess* ptr) 
        { 
            if (ptr) delete ptr; 
        }
    );
std::mutex PipelineProcess::s_mutex;
std::mutex PipelineProcess::callback_mutex;
std::once_flag PipelineProcess::s_onceFlag;
std::condition_variable PipelineProcess::m_cvEventQueue;
std::condition_variable PipelineProcess::m_cvCallbackQueue;
PipelineCallback PipelineProcess::m_callback = nullptr;
std::atomic<bool> PipelineProcess::g_processrunning = true;
std::atomic<bool> PipelineProcess::g_callbackrunning = true;

PipelineProcess& PipelineProcess::getInstance()
{
    std::call_once(s_onceFlag, []() 
        {
        s_instance.reset(new PipelineProcess());
        });
    return *s_instance;
}

bool PipelineProcess::initialize(const char* debugconfigPath, PipelineCallback callback)
{
    try
    {
        if (s_instance)
        {
            MX_LOG_INFO("PipelineProcess", "PipelineProcess already initialized");
            return true;
        }


        //////////////////////////////////////////////////////////////////////////////////////////////
        // Get the SystemInfoLogger instance
        auto& logger = SystemInfoLogger::getInstance();
        
        // Register all components that need to be initialized
        logger.registerComponent("Log Handler");
        logger.registerComponent("Pipeline Manager");
        logger.registerComponent("Event Queue");
        logger.registerComponent("Processing Thread");
        logger.registerComponent("Callback Queue");
        logger.registerComponent("Callback Thread");

        // Add SDK and library information
        logger.addLibraryInfo("PipelineSDK", "1.0.0", __DATE__ " " __TIME__);
        logger.addLibraryInfo("Poco", "1.9.4", "");
        
        // Register hardware capabilities
        #ifdef _WIN32
        logger.addHardwareCapability("OS", "Windows", Poco::Environment::osVersion());
        #else
        logger.addHardwareCapability("OS", "Linux", Poco::Environment::osVersion());
        #endif
        
        logger.addHardwareCapability("CPU Cores", std::to_string(Poco::Environment::processorCount()), "Available for processing");
        logger.addHardwareCapability("Memory", "Optimized", "Dynamic allocation based on workload");
        logger.addHardwareCapability("Video Processing", "Enabled", "H.264/H.265 codec support");
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        // set callback
        m_callback = std::move(callback);

        // Get the singleton instance
        PipelineProcess& instance = getInstance();
               

        // Step 1: Initialize the Logger
        MXLOGGER_STATUS_CODE status = MxLogger::instance().initialize(debugconfigPath);
        if (status != MXLOGGER_INIT_SUCCESSFULLY && status != MXLOGGER_STATUS_ALLREADY_INITIALIZED)
        {
            logger.updateComponentStatus("Log Handler", false, "Failed to initialize - error code: " + std::to_string(status));
            onManagerCallback(PipelineStatus::Information, 0, 0, "Pipeline logger initialization failed");
            
            // Since SystemInfoLogger already has auto-initialization, we don't return false here
            // to allow the pipeline to continue initializing
        }
        else
        {
            logger.updateComponentStatus("Log Handler", true, ("Successfully initialized with config: " + std::string(debugconfigPath)));
            MX_LOG_INFO("PipelineProcess", "Log Handler initialized successfully");
        }
       
        // Step 2: Create and initialize the Pipeline Manager
        logger.updateComponentStatus("Pipeline Manager", false, "Creating manager instance");

        instance.m_pipelineManager = std::make_unique<PipelineManager>();
        if (!instance.m_pipelineManager)
        {
            logger.updateComponentStatus("Pipeline Manager", false, "Failed to create manager instance");
            MX_LOG_ERROR("PipelineProcess", "Failed to create Pipeline Manager");
            return false;
        }
        
        bool managerInitialized = true ;
        instance.m_pipelineManager->initializemanager();

        if (!managerInitialized)
        {
            logger.updateComponentStatus("Pipeline Manager", false, "Manager initialization failed");
            MX_LOG_ERROR("PipelineProcess", "Pipeline Manager initialization failed");
            return false;
        }
        
        logger.updateComponentStatus("Pipeline Manager", true, "Manager initialized successfully");
        MX_LOG_INFO("PipelineProcess", "Pipeline Manager initialized successfully");
        

        // Step 3: Initialize Event Queue
        logger.updateComponentStatus("Event Queue", false, "Initializing event queue");
        instance.m_eventQueue = std::make_unique<std::queue<PipelineRequest>>();
        if (!instance.m_eventQueue)
        {
            logger.updateComponentStatus("Event Queue", false, "Failed to create event queue");
            MX_LOG_ERROR("PipelineProcess", "Failed to create Event Queue");
            return false;
        }
        
        logger.updateComponentStatus("Event Queue", true, "Event queue initialized successfully");
        logger.initializeReceiveQueue(true, "Event queue ready to receive requests");
        MX_LOG_INFO("PipelineProcess", "Event Queue initialized successfully");
        
        // Step 4: Initialize Callback Queue
        logger.updateComponentStatus("Callback Queue", false, "Initializing callback queue");
        instance.m_callbackQueue = std::make_unique<std::queue<CallbackData>>();
        if (!instance.m_callbackQueue)
        {
            logger.updateComponentStatus("Callback Queue", false, "Failed to create callback queue");
            MX_LOG_ERROR("PipelineProcess", "Failed to create Callback Queue");
            return false;
        }
        
        logger.updateComponentStatus("Callback Queue", true, "Callback queue initialized successfully");
        logger.initializeTransmitQueue(true, "Callback queue ready for transmitting results");
        MX_LOG_INFO("PipelineProcess", "Callback Queue initialized successfully");
        
        // Step 5: Start Processing Thread
        logger.updateComponentStatus("Processing Thread", false, "Starting processing thread");
        g_processrunning = true;
        instance.m_processingThread = std::make_unique<std::thread>(&PipelineProcess::processEvents, &instance);
        if (!instance.m_processingThread)
        {
            logger.updateComponentStatus("Processing Thread", false, "Failed to start processing thread");
            MX_LOG_ERROR("PipelineProcess", "Failed to create Processing Thread");
            return false;
        }
        
        logger.updateComponentStatus("Processing Thread", true, "Processing thread started successfully");
        MX_LOG_INFO("PipelineProcess", "Processing Thread started successfully");
        
        // Step 6: Start Callback Thread
        logger.updateComponentStatus("Callback Thread", false, "Starting callback thread");
        g_callbackrunning = true;
        instance.m_callbackThread = std::make_unique<std::thread>(&PipelineProcess::processCallbacks, &instance);
        if (!instance.m_callbackThread)
        {
            logger.updateComponentStatus("Callback Thread", false, "Failed to start callback thread");
            MX_LOG_ERROR("PipelineProcess", "Failed to create Callback Thread");
            return false;
        }
        
        logger.updateComponentStatus("Callback Thread", true, "Callback thread started successfully");
        MX_LOG_INFO("PipelineProcess", "Callback Thread started successfully");
        
       
        
        // Send acknowledgment to Video Manager if all components are initialized
        if (logger.areAllComponentsInitialized()) 
        {
            logger.sendAcknowledgmentToVideoManager();
            MX_LOG_INFO("PipelineProcess", "Initialization complete - sent acknowledgment to Video Manager");
        } 
        else 
        {
            MX_LOG_WARN("PipelineProcess", "Initialization complete but some components failed to initialize");
        }

        // Log the final system information with all components initialized
        logger.logSystemInfo();

        return true;
    }
    catch (const std::exception& e)
    {
        MX_LOG_ERROR("PipelineProcess", (std::string("Exception during initialization: ") + e.what()).c_str());
        return false;
    }
    catch (...)
    {
        MX_LOG_ERROR("PipelineProcess", "Unknown exception during initialization");
        return false;
    }
}

void PipelineProcess::processEvents()
{
    while (g_processrunning)
    {
        try
        {
            std::unique_lock<std::mutex> lock(s_mutex);
            m_cvEventQueue.wait(lock, [this]
                {
                    return !g_processrunning || !getInstance().m_eventQueue || !getInstance().m_eventQueue->empty();
                });

            if (!g_processrunning)
                break;

            if (getInstance().m_eventQueue && !getInstance().m_eventQueue->empty())
            {
                PipelineRequest request = getInstance().m_eventQueue->front();
                getInstance().m_eventQueue->pop();
                getInstance().m_pipelineManager->sendPipelineRequest(request);
            }
        }
        catch (const std::exception& e)
        {
            PipelineProcess::onManagerCallback(PipelineStatus::Success, 0, 0, std::string("Exception in process thread: ") + e.what());
        }
    }
}

void PipelineProcess::enqueueRequest(const PipelineRequest& request)
{
    try
    {
        {
            std::unique_lock<std::mutex> lock(s_mutex);
            getInstance().m_eventQueue->push(request);
        }
           
        m_cvEventQueue.notify_one();
        
        // Notify request enqueued
        onManagerCallback(PipelineStatus::InProgress,request.getPipelineID(),request.getRequestID(),"Incomming Request enqueued for in Pipeline Processor");

    }
    catch (const std::exception& e)
    {
        onManagerCallback(PipelineStatus::Error, 0, 0, std::string("Failed to enqueue request: ") + e.what());
    }
}

void PipelineProcess::shutdown()
{
    MX_LOG_TRACE("PipelineProcess", "Shutting down pipeline process");

    PipelineProcess::onManagerCallback(PipelineStatus::Success,
        0,  // No specific pipeline ID for shutdown
        0,  // No specific request ID for shutdown
        "Pipeline process shut down start");

    // Signal the processing thread to stop
    {
        std::lock_guard<std::mutex> lock(s_mutex); // Ensure thread safety
        g_processrunning = false;
        m_cvEventQueue.notify_all();
    }

    {
        std::lock_guard<std::mutex> lock(callback_mutex); // Ensure thread safety
        g_callbackrunning = false;
        m_cvCallbackQueue.notify_all();
    }

    // Wait for the processing thread to finish
    if (s_instance && s_instance->m_processingThread->joinable())
    {
        s_instance->m_processingThread->join();
    }

    // Wait for the callback thread to finish
    if (s_instance && s_instance->m_callbackThread->joinable())
    {
        s_instance->m_callbackThread->join();
    }
       
    // Safely clean up resources
    if (s_instance) 
    {
        {
            std::lock_guard<std::mutex> lock(callback_mutex); // Ensure cleanup thread-safe

            if (s_instance->m_callbackQueue)
            {
                s_instance->m_callbackQueue.reset();
            }
        }

        {
            std::lock_guard<std::mutex> lock(s_mutex); // Ensure cleanup thread-safe

            // Reset member pointers first
            if (s_instance->m_eventQueue)
            {
                s_instance->m_eventQueue.reset();
            }

            if (s_instance->m_pipelineManager)
            {
                s_instance->m_pipelineManager.reset();
            }
        }
        MxLogger::instance().shutdown();
    }
    // Now it's safe to reset the instance itself
    cleanupInstance();
}


void PipelineProcess::enqueueCallback(const CallbackData& data)
{
    try
    {
        {
            std::unique_lock<std::mutex> lock(callback_mutex);
            m_callbackQueue->push(data);
        }
        
        m_cvCallbackQueue.notify_one();
    }
    catch (const std::exception& e)
    {
        MX_LOG_ERROR("PipelineProcess", ("Failed to enqueue callback: " + std::string(e.what())).c_str());
    }
}

void PipelineProcess::processCallbacks()
{
    while (g_callbackrunning)
    {
        try
        {
            std::unique_lock<std::mutex> lock(callback_mutex);
            m_cvCallbackQueue.wait(lock, [this]
                {
                    return !g_callbackrunning || !m_callbackQueue || !m_callbackQueue->empty();
                });

            if (!g_callbackrunning)
                break;

            if (m_callbackQueue && !m_callbackQueue->empty())
            {
                CallbackData data = m_callbackQueue->front();
                m_callbackQueue->pop();
                
                // Release the lock before calling the callback to prevent deadlocks
                lock.unlock();
                
                // Call the actual callback
                if (m_callback)
                {
                    m_callback(data.status, data.pipelineId, data.requestId, data.message);
                }
            }
        }
        catch (const std::exception& e)
        {
            MX_LOG_ERROR("PipelineProcess", ("Exception in callback thread: " + std::string(e.what())).c_str());
        }
    }
}

void PipelineProcess::setCallback(PipelineCallback callback)
{
    m_callback = callback;
}

void PipelineProcess::onManagerCallback(PipelineStatus status, size_t pipelineId, size_t requestId, const std::string& message)
{
    // Process can add additional context here if needed
    if (m_callback)
    {
        // Queue the callback instead of calling it directly
        CallbackData data = {status, pipelineId, requestId, message};
        getInstance().enqueueCallback(data);
    }
}