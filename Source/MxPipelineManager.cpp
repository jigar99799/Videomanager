#include "MxPipelineManager.h"
#include "MediaStreamDevice.h"
#include <iostream>
#include <chrono>
#include <random>
#include <sstream>
#include <atomic>

// Use the scope resolution operator for PipelineID
using PipelineID = PipelineManager::PipelineID;

// Add this at the top of the file with other static members
std::atomic<PipelineID> PipelineManager::nextPipelineId{1};

// Constructor
PipelineManager::PipelineManager() : m_running(true) 
{
    // Initialize logger
    m_logger = std::make_unique<Logger>(Logger::OutputType::File, "pipeline_manager.log");
    m_logger->log("Pipeline Manager initialized");
    
    // Start the worker thread for processing queue
    m_workerThread = std::thread(&PipelineManager::processQueue, this);
}

// Destructor
PipelineManager::~PipelineManager() 
{
    m_running = false;
    m_cv.notify_all();
    
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    
    // Clear all pipelines
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pipelineHandlers.clear();
}

// Queue processing
void PipelineManager::processQueue() 
{

    while (m_running) 
    {
        MediaStreamDevice streamDevice;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { 
                return !m_running || !m_requestQueue.empty(); 
            });
            
            if (!m_running) break;
            
            streamDevice = m_requestQueue.front();
            m_requestQueue.pop();
        }
        
        std::cout << "Getting the first id : "  << std::endl;

        // Check for existing matching pipeline
        PipelineID existingId;
        if (findMatchingPipeline(streamDevice, existingId)) 
        {
            std::cout << "findMatchingPipeline" << std::endl;
            // Update existing pipeline if needed
            if (canUpdatePipeline(existingId, streamDevice)) 
            {
                updatePipelineInternal(existingId, streamDevice);
                m_logger->log("Updated existing pipeline: " + std::to_string(existingId));
            }
        } 
        else if (validatePipelineConfig(streamDevice)) 
        {

            std::cout << "validatePipelineConfig: " << std::endl;

            createPipelineInternal(streamDevice);
        } 
        else 
        {
            std::cout << "Invalid pipeline configuration received" << std::endl;
            m_logger->log("Invalid pipeline configuration received");
        }
    }
}

// Pipeline validation methods
bool PipelineManager::validatePipelineConfig(const MediaStreamDevice& streamDevice) const 
{
    // Validate source type and network configuration
    if (streamDevice.sourceType() == SourceType::NETWORK) {
        if (streamDevice.address().empty() || streamDevice.port() <= 0) {
            m_logger->log("Invalid network configuration: address or port missing");
            return false;
        }
    }
    
    // Validate codec configuration
    const MediaCodec& inputCodec = streamDevice.inputCodec();
    if (inputCodec.type == CodecType::H264 || inputCodec.type == CodecType::H265) {
        if (inputCodec.bitrate <= 0) {
            m_logger->log("Invalid codec configuration: bitrate not set");
            return false;
        }
    }
    
    return true;
}

bool PipelineManager::canUpdatePipeline(PipelineID id, const MediaStreamDevice& streamDevice) const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_pipelineHandlers.find(id);
    if (it == m_pipelineHandlers.end()) 
    {
        return false;
    }
    
    const MediaStreamDevice& existingConfig = it->second->getConfig();
    if (existingConfig.sourceType() != streamDevice.sourceType()) 
    {
        m_logger->log("Cannot update pipeline: source type mismatch");
        return false;
    }
    
    return true;
}

// Pipeline ID generation
PipelineID PipelineManager::generateUniquePipelineId() const {
    // Generate sequential IDs starting from 1
    return nextPipelineId.fetch_add(1, std::memory_order_relaxed);
}

bool PipelineManager::isPipelineExists(PipelineID id) const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pipelineHandlers.find(id) != m_pipelineHandlers.end();
}

// Pipeline creation and update methods
void PipelineManager::createPipelineInternal(const MediaStreamDevice& streamDevice) 
{



    
    PipelineID id = generateUniquePipelineId();

    std::lock_guard<std::mutex> lock(m_mutex);

    std::cout << "createPipelineInternal generateUniquePipelineId id = " << id <<std::endl;

    auto handler = std::make_unique<PipelineHandler>(streamDevice);
    
    // Set callbacks
    handler->setStateCallback([this, id](PipelineHandler::State state) 
        {
        std::string stateStr;
        switch (state) 
        {
            case PipelineHandler::State::PLAYING: stateStr = "PLAYING"; break;
            case PipelineHandler::State::PAUSED: stateStr = "PAUSED"; break;
            case PipelineHandler::State::STOPPED: stateStr = "STOPPED"; break;
            default: stateStr = "UNKNOWN"; break;
        }
        m_logger->log("Pipeline " + std::to_string(id) + " state changed to: " + stateStr);
    });
    
    handler->setErrorCallback([this, id](const std::string& error) 
    {
        m_logger->log("Pipeline " + std::to_string(id) + " error: " + error);
    });
    
    m_pipelineHandlers[id] = std::move(handler);
    m_logger->log("Created pipeline with ID: " + std::to_string(id));
}

void PipelineManager::updatePipelineInternal(PipelineID id, const MediaStreamDevice& streamDevice) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end()) 
    {
        if (it->second->updateConfiguration(streamDevice)) {
            m_logger->log("Successfully updated pipeline: " + std::to_string(id));
        } else {
            m_logger->log("Failed to update pipeline: " + std::to_string(id));
        }
    }
}

// Public interface implementations
PipelineID PipelineManager::createPipeline(const MediaStreamDevice& streamDevice) 
{
    PipelineID id = generateUniquePipelineId();
    enqueueRequest(streamDevice);
    std::cout << "createPipeline DOne 1" << std::endl;
    return id;
}

bool PipelineManager::updatePipeline(PipelineID id, const MediaStreamDevice& streamDevice) 
{
    if (!isPipelineExists(id)) return false;
    updatePipelineInternal(id, streamDevice);
    return true;
}

void PipelineManager::enqueueRequest(const MediaStreamDevice& streamDevice) 
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_requestQueue.push(streamDevice);
    }
    m_cv.notify_one();
}

// Pipeline control operations
bool PipelineManager::startPipeline(PipelineID id) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end()) 
    {
        return it->second->start();
    }
    return false;
}

bool PipelineManager::pausePipeline(PipelineID id) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end()) 
    {
        return it->second->pause();
    }
    return false;
}

bool PipelineManager::resumePipeline(PipelineID id) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end()) 
    {
        return it->second->resume();
    }
    return false;
}

bool PipelineManager::stopPipeline(PipelineID id) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end()) 
    {
        if (it->second->stop()) 
        {
            m_pipelineHandlers.erase(it);
            return true;
        }
    }
    return false;
}

bool PipelineManager::terminatePipeline(PipelineID id) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end()) 
    {
        it->second->terminate();
        m_pipelineHandlers.erase(it);
        return true;
    }
    return false;
}

// Status queries
bool PipelineManager::isPipelineRunning(PipelineID id) const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end()) 
    {
        return it->second->isRunning();
    }
    return false;
}

std::vector<PipelineID> PipelineManager::getActivePipelines() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<PipelineID> activePipelines;
    for (const auto& [id, handler] : m_pipelineHandlers) 
    {
        if (handler->isRunning()) 
        {
            activePipelines.push_back(id);
        }
    }
    return activePipelines;
}

size_t PipelineManager::getQueueSize() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_requestQueue.size();
}

// Logger initialization
bool PipelineManager::initializeLogger(const std::string& loggerPath) 
{
    try 
    {
        // TODO: Implement DLL loading logic
        /* Example pseudo-code for future implementation:
        HMODULE hDLL = LoadLibrary(loggerPath.c_str());
        if (hDLL) {
            auto initFunc = GetProcAddress(hDLL, "InitializeLogger");
            auto logFunc = GetProcAddress(hDLL, "LogMessage");
            // Initialize logger with configuration
        }
        */
        
        m_logger->log("Logger DLL initialized successfully");
        return true;
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        return false;
    }
}

bool PipelineManager::findMatchingPipeline(const MediaStreamDevice& streamDevice, PipelineID& outId) const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& [id, handler] : m_pipelineHandlers) {
        const MediaStreamDevice& existingConfig = handler->getConfig();
        
        if (existingConfig.sourceType() == streamDevice.sourceType() &&
            existingConfig.mediaType() == streamDevice.mediaType() &&
            existingConfig.address() == streamDevice.address() &&
            existingConfig.port() == streamDevice.port()) {
            
            outId = id;
            return true;
        }
    }
    
    return false;
}
