#include "PipelineProcess.h"

// Define static members
std::unique_ptr<PipelineProcess, std::function<void(PipelineProcess*)>> PipelineProcess::s_instance = nullptr;
std::mutex PipelineProcess::s_mutex;
std::once_flag PipelineProcess::s_onceFlag;
std::condition_variable PipelineProcess::m_cvEventQueue;
processCallback PipelineProcess::m_processCallbackfn = nullptr;
std::atomic<bool> PipelineProcess::g_processrunning = true;

PipelineProcess& PipelineProcess::getInstance()
{
    std::call_once(s_onceFlag, []() 
        {
        s_instance.reset(new PipelineProcess());
        });
    return *s_instance;
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

            if (!g_processrunning || !getInstance().m_eventQueue)
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
        PipelineProcess& pipelineinstance = getInstance(); 

        std::lock_guard<std::mutex> lock(s_mutex);

        //set callback F
        m_processCallbackfn = std::move(callback);

//Logger_init
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



//pipeline_manager_init

        // Create pipeline manager
        pipelineinstance.m_pipelineManager = std::make_unique<PipelineManager>();
        pipelineinstance.m_pipelineManager->initializemanager();

        MX_LOG_INFO("PipelineProcess", "Pipeline manager initialized");



//event_queue_init
        
        // Create event queue
        pipelineinstance.m_eventQueue = std::make_unique<TQueue<PipelineRequest>>();
        MX_LOG_INFO("PipelineProcess", "Event queue initialized");


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
    MX_LOG_TRACE("PipelineProcess", "pipe line process shutdown start");

    {
        std::lock_guard<std::mutex> lock(s_mutex); // Ensure thread safety
        g_processrunning = false;
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
    s_instance.reset();  // Cleanup singleton
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
