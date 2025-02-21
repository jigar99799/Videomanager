#include "PipelineProcess.h"

// Define static members
std::unique_ptr<PipelineProcess> PipelineProcess::s_instance = nullptr;
std::mutex PipelineProcess::s_mutex;
std::condition_variable PipelineProcess::m_cvEventQueue;
processCallback PipelineProcess::m_processCallbackfn = nullptr;

PipelineProcess& PipelineProcess::getInstance()
{
    if (!s_instance)
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_instance = std::unique_ptr<PipelineProcess>(new PipelineProcess());
    }
    return *s_instance;
}

void PipelineProcess::processEvents()
{
    while (g_running)
    {
        try
        {
            std::unique_lock<std::mutex> lock(s_mutex);
            m_cvEventQueue.wait(lock, [this]
                {
                    return !g_running || !getInstance().m_eventQueue || !getInstance().m_eventQueue->isEmpty();
                });

            if (!g_running || !getInstance().m_eventQueue) 
                break; // Safeguard shutdown

            PipelineRequest request = getInstance().m_eventQueue->dequeue();
            getInstance().m_pipelineManager->sendPipelineRequest(request);

            MX_LOG_INFO("PipelineProcess", ("Processing request for pipeline: " + std::to_string(request.getPipelineID())).c_str());
        }
        catch (const std::exception& e)
        {
            MX_LOG_ERROR("PipelineProcess", ("Error processing event: " + std::string(e.what())).c_str());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


bool PipelineProcess::initialize(const char* debugconfigPath, processCallback callback)
{
    try 
    {
        if (s_instance)
        {
            MX_LOG_INFO("PipelineProcess", "PipelineProcess allready init");
            return true;
        }

        // Ensures the instance is created
        getInstance();

        std::lock_guard<std::mutex> lock(s_mutex);

        //set callback 
        m_processCallbackfn = std::move(callback);

#pragma region Logger_init
        // Initialize logger first

        MXLOGGER_STATUS_CODE status = MxLogger::instance().initialize(debugconfigPath);
        if (status != MXLOGGER_INIT_SUCCESSFULLY && status != MXLOGGER_STATUS_ALLREADY_INITIALIZED)
        {
            if (m_processCallbackfn)
                m_processCallbackfn(MXPIPELINE_PROCESS_LOGGER_INIT_FAILED, "with failed status :" + std::to_string((int)status));

            // TODO : this failed is not important 
           //return false;
        }
        else
            MX_LOG_INFO("PipelineProcess", "Logger initialized successfully");

#pragma endregion Logger_init

#pragma region pipeline_manager_init

        // Create pipeline manager
        getInstance().m_pipelineManager = std::make_unique<PipelineManager>();
        getInstance().m_pipelineManager->initializemanager();

        MX_LOG_INFO("PipelineProcess", "Pipeline manager initialized");

#pragma endregion pipeline_manager_init

#pragma region event_queue_init
        
        // Create event queue
        getInstance().m_eventQueue = std::make_unique<TQueue<PipelineRequest>>();
        MX_LOG_INFO("PipelineProcess", "Event queue initialized");

#pragma endregion event_queue_init

        return true;
    }
    catch (const std::exception& e) 
    {
        if (m_processCallbackfn)
            m_processCallbackfn(MXPIPELINE_PROCESS_LOGGER_INIT_FAILED, "Initialization failed :" + std::string(e.what()));
        MX_LOG_ERROR("PipelineProcess", ("Initialization failed: " + std::string(e.what())).c_str());
        return false;
    }
}

void PipelineProcess::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(s_mutex); // Ensure thread safety
        g_running = false;
        m_cvEventQueue.notify_all();
    }

    if (getInstance().m_processingThread.joinable())
    {
        getInstance().m_processingThread.join();
    }

    {
        std::lock_guard<std::mutex> lock(s_mutex); // Ensure cleanup  thread-safe
        getInstance().m_eventQueue.reset();
        getInstance().m_pipelineManager.reset();
        MxLogger::instance().shutdown();
    }
}

void PipelineProcess::enqueueRequest(const PipelineRequest& request)
{
    if (getInstance().m_eventQueue)
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        getInstance().m_eventQueue->enqueue(request);
        MX_LOG_TRACE("PipelineProcess", ("Request enqueued for pipeline: " + std::to_string(request.getPipelineID())).c_str());
    }
    m_cvEventQueue.notify_one();
}
