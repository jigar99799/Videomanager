#pragma once

#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <iostream>
#include <functional>
#include <queue>
#include <memory>
#include "PipelineManager.h"
#include "PipelineRequest.h"
#include "mx_logger.h"

// Define the callback type that will be exposed to main
using PipelineCallback = std::function<void(
    PipelineStatus status,          // Current status
    size_t pipelineId,             // ID of the affected pipeline
    size_t requestId,              // ID of the original request
    const std::string& message     // Detailed message
)>;

class PipelineManager;

class PipelineProcess 
{
private:
    static std::unique_ptr<PipelineProcess, std::function<void(PipelineProcess*)>> s_instance;
    static std::mutex               s_mutex;
    static std::condition_variable  m_cvEventQueue;
    static PipelineCallback         m_callback;  // Callback to main
    static std::once_flag           s_onceFlag;
    static std::atomic<bool>        g_processrunning;
    static std::condition_variable  m_cvCallbackQueue;

    // Structure to hold callback data
    struct CallbackData {
        PipelineStatus status;
        size_t pipelineId;
        size_t requestId;
        std::string message;
    };

    std::unique_ptr<PipelineManager>                m_pipelineManager;
    std::unique_ptr<std::queue<PipelineRequest>>    m_eventQueue;
    std::unique_ptr<std::queue<CallbackData>>       m_callbackQueue;
    std::unique_ptr<std::thread>                    m_processingThread;
    std::unique_ptr<std::thread>                    m_callbackThread;
    static std::atomic<bool>        g_callbackrunning;
    static std::mutex               callback_mutex;
   
    // Private constructor to prevent instantiation
    PipelineProcess() 
    {
        /*m_processingThread = std::thread(&PipelineProcess::processEvents,this);*/
        /*g_processrunning = true;*/
    }

    ~PipelineProcess()
    {
        // Don't call shutdown() here to avoid circular dependency
        // Resources will be cleaned up by the shutdown() method called externally
    }

    void processEvents();
    void processCallbacks();
    
    // Handle pipeline errors and propagate them to application
   // static  void PipelineErrorcallback(const std::string& error);

    // Internal callback from Manager to Process
    static void onManagerCallback(PipelineStatus status, size_t pipelineId,
                         size_t requestId, const std::string& message);
    
public:
    // Delete copy constructor and assignment operator
    PipelineProcess(const PipelineProcess&) = delete;
    PipelineProcess& operator=(const PipelineProcess&) = delete;

    static PipelineProcess& getInstance();
    static bool initialize(const char* debugconfigPath, PipelineCallback callback);
    
    static void shutdown();
    static void enqueueRequest(const PipelineRequest& request);
    
    // Set callback for pipeline status updates and errors
    static void setCallback(PipelineCallback callback);
    
    // Helper method to enqueue a callback
    void enqueueCallback(const CallbackData& data);

    // Clean up the singleton instance - call this at application exit
    static void cleanupInstance() {
        s_instance.reset();
    }
};
