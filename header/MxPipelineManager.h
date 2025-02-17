
#if 0
// CMxPipeManager.h
#pragma once

#include <gst/gst.h>
#include <unordered_map>
#include <string>
#include "Pipeline.h"  // Include your Pipeline structure definition
#include "TQueue.h"    // Include your TQueue definition

class CMxPipeManager {
public:
    void addPipeline(const Pipeline& pipeline);
    void handlePipelineAction(const std::string& pipelineId, const std::string& requestId, const std::string& action);
    
private:
    TQueue<Pipeline> parsedQueue; // Queue to hold pipeline data
    std::unordered_map<std::string, GstElement*> pipelines; // Map to track pipelines by ID
public:
    void createPipeline(const Pipeline& currentPipeline);
    void startPipeline(const std::string& pipelineId);
    void stopPipeline(const std::string& pipelineId);
    void pausePipeline(const std::string& pipelineId);
    void processQueue();

    static void on_pad_added(GstElement* src, GstPad* pad, gpointer data);
};
#endif


#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H

#include "PipelineHandler.h"
#include <unordered_map>
#include <queue>
#include <thread>
#include <mutex>

class PipelineManager {
private:
    std::unordered_map<size_t, PipelineHandler*> m_pipelineHandlers;
    std::queue<Pipeline> m_pipelineQueue;
    std::mutex m_mutex;
    bool m_running;
    std::thread m_workerThread;

    void processQueue();
    void createPipelineInternal(const Pipeline& pipeline);

public:
    PipelineManager();
    ~PipelineManager();

    void enqueuePipeline(const Pipeline& pipeline);
    void startPipeline(size_t pipelineID);
    void pausePipeline(size_t pipelineID);
    void stopPipeline(size_t pipelineID);
};

#endif  // PIPELINE_MANAGER_H
