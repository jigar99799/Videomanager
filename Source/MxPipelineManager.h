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

class PipelineManager {
public:
    using PipelineID = uint64_t;
    
    PipelineManager();
    ~PipelineManager();
    
    // Delete copy and move operations
    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;
    PipelineManager(PipelineManager&&) = delete;
    PipelineManager& operator=(PipelineManager&&) = delete;

    // Pipeline management
    PipelineID createPipeline(const MediaStreamDevice& streamDevice);
    bool updatePipeline(PipelineID id, const MediaStreamDevice& streamDevice);
    
    // Pipeline control operations
    bool startPipeline(PipelineID id);
    bool pausePipeline(PipelineID id);
    bool resumePipeline(PipelineID id);
    bool stopPipeline(PipelineID id);
    bool terminatePipeline(PipelineID id);
    bool isPipelineRunning(PipelineID id) const;
    
    // Queue management
    void enqueueRequest(const MediaStreamDevice& streamDevice);
    size_t getQueueSize() const;
    std::vector<PipelineID> getActivePipelines() const;

    // Logger initialization
    bool initializeLogger(const std::string& loggerPath);

private:
    static std::atomic<PipelineID> nextPipelineId;
    using PipelineHandlerPtr = std::unique_ptr<PipelineHandler>;
    
    // Member variables
    std::unordered_map<PipelineID, PipelineHandlerPtr> m_pipelineHandlers;
    std::queue<MediaStreamDevice> m_requestQueue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{true};
    std::thread m_workerThread;
    std::unique_ptr<Logger> m_logger;

    // Private helper methods
    void processQueue();
    PipelineID generateUniquePipelineId() const;
    bool isPipelineExists(PipelineID id) const;
    void createPipelineInternal(const MediaStreamDevice& streamDevice);
    void updatePipelineInternal(PipelineID id, const MediaStreamDevice& streamDevice);
    bool validatePipelineConfig(const MediaStreamDevice& streamDevice) const;
    bool canUpdatePipeline(PipelineID id, const MediaStreamDevice& streamDevice) const;
    bool findMatchingPipeline(const MediaStreamDevice& streamDevice, PipelineID& existingId) const;
};

#endif // PIPELINE_MANAGER_H
