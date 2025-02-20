#pragma once

#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <iostream>
#include <functional>
#include "MxPipelineManager.h"
#include "TQueue.h"
#include "PipelineRequest.h"
#include "mx_logger.h"

// Global flag for program control
std::atomic<bool> g_running{ true };

using processCallback = std::function<void(MXPIPELINE_PROCESS_STATUS_CODE,std::string status)>;

class PipelineProcess 
{
private:
    static std::unique_ptr<PipelineProcess> s_instance;
    static std::mutex s_mutex;
    static std::condition_variable m_cvEventQueue;
    static processCallback m_processCallbackfn;

    std::unique_ptr<PipelineManager> m_pipelineManager;
    std::unique_ptr<TQueue<PipelineRequest>> m_eventQueue;
    std::thread m_processingThread;
   
    // Private constructor to prevent instantiation
    PipelineProcess() :m_pipelineManager(std::make_unique<PipelineManager>()),m_eventQueue(std::make_unique<TQueue<PipelineRequest>>())
    {
        m_processingThread = std::thread(&PipelineProcess::processEvents);
    }

    ~PipelineProcess()
    {
        g_running = false;
        if (m_processingThread.joinable())
        {
            m_processingThread.join();
        }
    }

    void processEvents();

public:
    // Delete copy constructor and assignment operator
    PipelineProcess(const PipelineProcess&) = delete;
    PipelineProcess& operator=(const PipelineProcess&) = delete;

    static PipelineProcess& getInstance();

    static bool initialize(const char* debugconfigPath, processCallback callback);
    static void shutdown();
    static void enqueueRequest(const PipelineRequest& request);
};
