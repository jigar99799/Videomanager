#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H

#include <queue>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <condition_variable>
#include "Logger.h"
#include "Struct.h"
#include "PipelineHandler.h"
#include "MediaStreamDevice.h"
#include "TQueue.h"
#include "PipelineRequest.h"

#define PipelineID size_t

//
//struct PipelineRequest 
//{
//    PipelineOperation operation;
//    PipelineID id;
//    MediaStreamDevice streamDevice;
//};


class PipelineManager 
{
private:
    using PipelineHandlerPtr = std::unique_ptr<PipelineHandler>;

    // Member variables
    std::unordered_map<PipelineID, PipelineHandlerPtr> m_pipelineHandlers;
    TQueue<PipelineRequest>        m_pipelinerequest;
    static std::atomic<PipelineID> nextPipelineId;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{ true };
    std::thread m_workerThread;

    //   Pipeline comparison  
    bool findMatchingPipeline(const MediaStreamDevice& streamDevice, PipelineID& existingId) const;
    bool validatePipelineConfig(const MediaStreamDevice& streamDevice) const;
    bool isPipelineExists(PipelineID id) const;
    bool canUpdatePipeline(PipelineID id, const MediaStreamDevice& streamDevice) const;

    //   Request Process
    void startworkerthread();
    void stopworkerthread();
    void processpipelinerequest();
    void enqueuePipelineRequest(const PipelineRequest& request);

    //   Control operations
    void createPipelineInternal(PipelineID id, const MediaStreamDevice& streamDevice);
    void updatePipelineInternal(PipelineID id, const MediaStreamDevice& streamDevice);
    bool startPipeline(PipelineID id);
    bool pausePipeline(PipelineID id);
    bool resumePipeline(PipelineID id);
    bool stopPipeline(PipelineID id);
    bool terminatePipeline(PipelineID id);

    // generate Pipeline ID
    PipelineID generatePipelineId() const;

public:
    
    PipelineManager();
    ~PipelineManager();

    // Logger initialization
    bool initializeLogger(const char* configPath);

    // Pipeline initialization 
    void initializemanager();

    PipelineID createPipeline     (const MediaStreamDevice& streamDevice);
    void       sendPipelineRequest(PipelineRequest incommingrequest);

    // Delete copy and move operations
    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;
    PipelineManager(PipelineManager&&) = delete;
    PipelineManager& operator=(PipelineManager&&) = delete;

    // Pipeline status
    bool isPipelineRunning(PipelineID id) const;
    size_t getQueueSize() const;
    std::vector<PipelineID> getActivePipelines() const;

};

#endif // PIPELINE_MANAGER_H
