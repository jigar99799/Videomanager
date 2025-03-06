#include "PipelineProcess.h"
#include "mx_systemInfoLogger.h"

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
            MX_LOG_INFO("PipelineProcess", "PipelineProcess allready init");
            return true;
        }

        SystemInfoLogger::getInstance().logSystemInfo();

        // set callback
        m_callback = std::move(callback);

        // Get the singleton instance
        PipelineProcess& instance = getInstance();
               
        // Step :1 init the Logger 

        MXLOGGER_STATUS_CODE status = MxLogger::instance().initialize(debugconfigPath);
        if (status != MXLOGGER_INIT_SUCCESSFULLY && status != MXLOGGER_STATUS_ALLREADY_INITIALIZED)
        {
            onManagerCallback(PipelineStatus::Information, 0, 0, "Pipeline logger initialized failed ");
            
            // TODO : this failed is not important 
           //return false;
        }
        else
        {
            MX_LOG_INFO("PipelineProcess", "Logger initialized successfully");
        }
       
        // Step :2 Create and init the Pipeline Manager 
        instance.m_pipelineManager = std::make_unique<PipelineManager>();

        if (instance.m_pipelineManager == NULL)
        {
            MX_LOG_TRACE("PipelineProcess", "Pipeline manager creation failed");

            PipelineProcess::onManagerCallback(PipelineStatus::Error, 0, 0, std::string("Pipeline manager creation failed"));

            //PipelineErrorcallback("Pipeline manager creation failed");
            return false;
        }

        instance.m_pipelineManager->initializemanager();

        // Set up the manager callback
        instance.m_pipelineManager->setManagerCallback(
            std::bind(&PipelineProcess::onManagerCallback, 
                     std::placeholders::_1,
                     std::placeholders::_2,
                     std::placeholders::_3,
                     std::placeholders::_4));

        // Step 3 : start the worker thread for processing the incomming request
        instance.m_processingThread = std::thread(&PipelineProcess::processEvents, &instance);

        // Start the callback processing thread
        instance.m_callbackThread = std::thread(&PipelineProcess::processCallbacks, &instance);
        g_processrunning = true;

        // Create event queue
        instance.m_eventQueue = std::make_unique<TQueue<PipelineRequest>>();

        // Create callback queue
        instance.m_callbackQueue = std::make_unique<TQueue<CallbackData>>();
        g_callbackrunning = true;

        MX_LOG_INFO("PipelineProcess", "Event queue initialized");

        onManagerCallback(PipelineStatus::Success, 0, 0, "Pipeline process initialized successfully");

        return true;
    }
    catch (const std::exception& e)
    {
        onManagerCallback(PipelineStatus::Error, 0, 0, std::string("Failed to initialize pipeline process: ") + e.what());
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
                    return !g_processrunning || !getInstance().m_eventQueue || !getInstance().m_eventQueue->isEmpty();
                });

            if (!g_processrunning)
                break;

            if (getInstance().m_eventQueue && !getInstance().m_eventQueue->isEmpty())
            {
                PipelineRequest request = getInstance().m_eventQueue->dequeue();
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
            getInstance().m_eventQueue->enqueue(request);
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
    if (s_instance && s_instance->m_processingThread.joinable())
    {
        s_instance->m_processingThread.join();
    }

    // Wait for the callback thread to finish
    if (s_instance && s_instance->m_callbackThread.joinable())
    {
        s_instance->m_callbackThread.join();
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
            m_callbackQueue->enqueue(data);
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
                    return !g_callbackrunning || !m_callbackQueue || !m_callbackQueue->isEmpty();
                });

            if (!g_callbackrunning)
                break;

            if (m_callbackQueue && !m_callbackQueue->isEmpty())
            {
                CallbackData data = m_callbackQueue->dequeue();
                
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