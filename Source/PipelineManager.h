#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H

#include <queue>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <condition_variable>
#include <functional>
#include <string>
#include "Struct.h"
#include "PipelineHandler.h"
#include "MediaStreamDevice.h"
#include "TQueue.h"
#include "PipelineRequest.h"

// Forward declare PipelineStatus enum from PipelineProcess.h
enum class PipelineStatus;

class PipelineHandler;

#define PipelineID size_t

// Define the callback type for manager (same signature as PipelineProcess's callback)
using ManagerCallback = std::function<void(
    PipelineStatus status,
    size_t pipelineId,
    size_t requestId,
    const std::string& message
)>;


class PipelineManager 
{
private:
    using PipelineHandlerPtr = std::unique_ptr<PipelineHandler>;

    // Member variables
    std::unordered_map<PipelineID, PipelineHandlerPtr> m_pipelineHandlers;
    TQueue<PipelineRequest>        m_pipelinerequest;
    
    // Thread management
    std::thread             m_workerThread;
    std::mutex              m_pipemangermutex;
    std::condition_variable m_cv;
    std::atomic<bool>       m_running{true};
    
    // Manager's callback
    ManagerCallback         m_callback;

    //   Pipeline comparison  
    bool findMatchingpipeline  (const MediaStreamDevice& streamDevice, PipelineID& existingId);
    bool validatepipelineConfig(const MediaStreamDevice& streamDevice) const;
    bool ispipelineexists (PipelineID id);
    bool canUpdatePipeline(PipelineID id, const MediaStreamDevice& streamDevice);
    
    //   Request Process   
    void startworkerthread();
    void stopworkerthread ();
    void enqueuePipelineRequest(const PipelineRequest& request);
    void processpipelinerequest();
    
    //   Control operations  
    void createPipelineInternal(PipelineID id, size_t iRequestID, const MediaStreamDevice& streamDevice);
    void updatePipelineInternal(PipelineID id, const MediaStreamDevice& streamDevice);
    bool startPipeline (PipelineID id);
    bool pausePipeline (PipelineID id);
    bool resumePipeline(PipelineID id);
    bool stopPipeline  (PipelineID id);
    bool terminatePipeline(PipelineID id);

    // Internal callback from Handler to Manager
    void onHandlerCallback(PipelineStatus status, size_t pipelineId, 
                         size_t requestId, const std::string& message);

public:
    
    PipelineManager();
    ~PipelineManager();
    
    void initializemanager();
    
    // Alias for setManagerCallback for compatibility with PipelineProcess
    void setManagerCallback(ManagerCallback callback)
    {
        m_callback = callback;
    }
    
    // Send pipeline request
    void sendPipelineRequest(PipelineRequest incommingrequest);
    
    // Pipeline Status queries
    bool isPipelineRunning(PipelineID id);
    std::vector<PipelineID> getActivePipelines();
    size_t getQueueSize();

    // Delete copy and move operations
    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;
    PipelineManager(PipelineManager&&) = delete;
    PipelineManager& operator=(PipelineManager&&) = delete;
};

#endif // PIPELINE_MANAGER_H
