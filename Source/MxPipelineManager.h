#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H

#include "PipelineHandler.h"
#include "MediaStreamDevice.h"
#include <unordered_map>
#include <queue>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <condition_variable>
#include "Logger.h"
#include "Struct.h"


using PipelineID = uint64_t;

struct PipelineRequest 
{
    PipelineOperation operation;
    PipelineID id;
    MediaStreamDevice streamDevice;
};


class PipelineManager {
public:
   
    
    PipelineManager();
    ~PipelineManager();
    
    // Delete copy and move operations
    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;
    PipelineManager(PipelineManager&&) = delete;
    PipelineManager& operator=(PipelineManager&&) = delete;

    void sendPipelineRequest(PipelineID id, const MediaStreamDevice& config, PipelineOperation op);


    // Pipeline management
    PipelineID createPipeline(const MediaStreamDevice& streamDevice);
    bool updatePipeline(PipelineID id, const MediaStreamDevice& streamDevice);
    
    
    bool isPipelineRunning(PipelineID id) const;
    
    // Queue management
    void enqueueRequest(const MediaStreamDevice& streamDevice);
    void enqueuePipelineRequest(const PipelineRequest& request);

    size_t getQueueSize() const;
    std::vector<PipelineID> getActivePipelines() const;

    // Logger initialization
    bool initializeLogger(const char* configPath);

private:
    static std::atomic<PipelineID> nextPipelineId;
    using PipelineHandlerPtr = std::unique_ptr<PipelineHandler>;
    
    // Member variables
    std::unordered_map<PipelineID, PipelineHandlerPtr> m_pipelineHandlers;
    std::queue<MediaStreamDevice> m_requestQueue;

    std::queue<PipelineRequest> m_pipelinerequest;
    void processpipelinerequest();

    // Pipeline control operations
    bool startPipeline(PipelineID id);
    bool pausePipeline(PipelineID id);
    bool resumePipeline(PipelineID id);
    bool stopPipeline(PipelineID id);
    bool terminatePipeline(PipelineID id);




    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{true};
    std::thread m_workerThread;

    // Private helper methods
    void processQueue();
  

    PipelineID generateUniquePipelineId() const;
    bool isPipelineExists(PipelineID id) const;
    void createPipelineInternal(PipelineID id ,const MediaStreamDevice& streamDevice);
    void updatePipelineInternal(PipelineID id, const MediaStreamDevice& streamDevice);
    bool validatePipelineConfig(const MediaStreamDevice& streamDevice) const;
    bool canUpdatePipeline(PipelineID id, const MediaStreamDevice& streamDevice) const;
    bool findMatchingPipeline(const MediaStreamDevice& streamDevice, PipelineID& existingId) const;
};

#endif // PIPELINE_MANAGER_H
