#include "MxPipelineManager.h"
#include "MediaStreamDevice.h"
#include <iostream>
#include <chrono>
#include <random>
#include <sstream>
#include <atomic>
#include "mx_logger.h"

// Add this at the top of the file with other static members
std::atomic<PipelineID> PipelineManager::nextPipelineId{1};

// Constructor
PipelineManager::PipelineManager() 
{
   
}

// Destructor
PipelineManager::~PipelineManager() 
{
    stopworkerthread();

    // Clear all pipelines
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pipelineHandlers.clear();
}

///////////////////////////////////////////////    Initialization   ///////////////////////////////////////////////

void PipelineManager::initializemanager()
{
    startworkerthread();
}

bool PipelineManager::initializeLogger(const char* configPath)
{
    try
    {
        MXLOGGER_STATUS_CODE status = MxLogger::instance().initialize(configPath);

        if (status != MXLOGGER_INIT_SUCCESSFULLY && status != MXLOGGER_STATUS_ALLREADY_INITIALIZED)
        {
            std::cerr << "Warning: Failed to initialize logger with status" << status << std::endl;
            return false;
        }

        MX_LOG_TRACE("PipelineManager", "logger DLL initialized successfully");

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

PipelineID PipelineManager::generatePipelineId() const
{
    // Generate sequential IDs starting from 1
    return nextPipelineId.fetch_add(1, std::memory_order_relaxed);
}

///////////////////////////////////////////////    Pipeline Interface   //////////////////////////////////////////

PipelineID PipelineManager::createPipeline(const MediaStreamDevice& streamDevice)
{
    // Generate new ID and create pipeline immediately
    PipelineID id = generatePipelineId();
    try
    {
        // Create a request with both ID and stream device
        PipelineRequest request{ PipelineOperation::Create, id, streamDevice };

        enqueuePipelineRequest(request);

        MX_LOG_INFO("PipelineManager", ("enqueue pipeline ID: " + std::to_string(id) + " for device: " + streamDevice.name()).c_str());
        return id;
    }
    catch (const std::exception& e)
    {
        MX_LOG_ERROR("PipelineHandler", ("failed to create pipeline: " + streamDevice.sDeviceName + e.what()).c_str());
        throw;
    }
}

void PipelineManager::sendPipelineRequest(PipelineRequest incommingrequest)
{
    enqueuePipelineRequest(incommingrequest);
}

///////////////////////////////////////////////    Pipeline comparison   //////////////////////////////////////////

bool PipelineManager::findMatchingPipeline(const MediaStreamDevice& streamDevice, PipelineID& outId) const
{
    // TODO : check all data ?? 
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& [id, handler] : m_pipelineHandlers)
    {
        const MediaStreamDevice& existingConfig = handler->getConfig();

        if (existingConfig.getinputmediadata().esourceType == streamDevice.getinputmediadata().esourceType &&
            existingConfig.getinputmediadata().stMediaCodec == existingConfig.getinputmediadata().stMediaCodec &&
            existingConfig.getinputmediadata().stNetworkStreaming.sIpAddress == streamDevice.getinputmediadata().stNetworkStreaming.sIpAddress &&
            existingConfig.getinputmediadata().stNetworkStreaming.iPort == streamDevice.getinputmediadata().stNetworkStreaming.iPort)
        {

            outId = id;
            MX_LOG_TRACE("PipelineManager", ("matching pipeline found ID: " + std::to_string(outId)).c_str());
            return true;
        }
    }

    MX_LOG_TRACE("PipelineManager", ("matching pipeline not found for devide : " + streamDevice.name()).c_str());
    return false;
}

bool PipelineManager::validatePipelineConfig(const MediaStreamDevice& streamDevice) const
{
    // Validate source type and network configuration
    if (streamDevice.getinputmediadata().esourceType == eSourceType::SOURCE_TYPE_NETWORK)
    {
        if (streamDevice.getinputmediadata().stNetworkStreaming.sIpAddress.empty() || streamDevice.getinputmediadata().stNetworkStreaming.iPort <= 0)
        {
            MX_LOG_ERROR("PipelineManager", "Invalid network configuration: address or port missing");
            return false;
        }
    }

    // Validate codec configuration
    if (streamDevice.getinputmediadata().stMediaCodec.type == CodecType::H264 || streamDevice.getinputmediadata().stMediaCodec.type == CodecType::H265)
    {
        if (streamDevice.getinputmediadata().stMediaCodec.bitrate <= 0)
        {
            MX_LOG_ERROR("PipelineManager", "Invalid codec configuration: bitrate not set");
            return false;
        }
    }

    return true;
}

bool PipelineManager::isPipelineExists(PipelineID id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pipelineHandlers.find(id) != m_pipelineHandlers.end();
}

bool PipelineManager::canUpdatePipeline(PipelineID id, const MediaStreamDevice& streamDevice) const
{
    // TODO  : what to allow and what not to
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_pipelineHandlers.find(id);
    if (it == m_pipelineHandlers.end())
    {
        return false;
    }

    const MediaStreamDevice& existingConfig = it->second->getConfig();
    if (existingConfig.getinputmediadata() != streamDevice.getinputmediadata())
    {
        MX_LOG_ERROR("PipelineManager", "Cannot update pipeline: input source type mismatch");
        return false;
    }

    return true;
}

///////////////////////////////////////////////    Request Process   //////////////////////////////////////////

void PipelineManager::startworkerthread()
{
    m_running = true;
    m_workerThread = std::thread(&PipelineManager::processpipelinerequest, this);
}

void PipelineManager::stopworkerthread()
{
    m_running = false;
    m_cv.notify_all();

    if (m_workerThread.joinable())
    {
        m_workerThread.join();
    }
}

void PipelineManager::enqueuePipelineRequest(const PipelineRequest& request)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pipelinerequest.enqueue(request);
    }
    m_cv.notify_one();
}

void PipelineManager::processpipelinerequest()
{
    m_running = true;

    while (m_running)
    {
        PipelineRequest request;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]
                {
                    return !m_running || !m_pipelinerequest.isEmpty();
                });

            if (!m_running) break;

            request = m_pipelinerequest.dequeue();
        }

        if (request.operation == PipelineOperation::Create ||
            request.operation == PipelineOperation::Update ||
            request.operation == PipelineOperation::Run)
        {
            if (!validatePipelineConfig(request.streamDevice))
            {
                MX_LOG_ERROR("PipelineManager", ("invalid configuration received for device :" + request.streamDevice.name()).c_str());
                continue;
            }
        }

        try
        {
            switch (request.operation)
            {
            case PipelineOperation::Create:
            {
                PipelineID  existingId;
                if (findMatchingPipeline(request.streamDevice, existingId))
                {
                    MX_LOG_INFO("PipelineManager", ("match same pipeline so can't created :" + std::to_string(existingId)).c_str());
                    // TODO : what to do blindlly created or return 
                    break;
                }
                createPipelineInternal(request.id, request.streamDevice);
                break;
            }
            case PipelineOperation::Update:
            {
                PipelineID  existingId;
                if (findMatchingPipeline(request.streamDevice, existingId))
                {
                    MX_LOG_INFO("PipelineManager", ("match same pipeline mot found while updatating for device :" + request.streamDevice.name()).c_str());
                    // TODO : what to do blindlly created or return 
                    break;
                }

                if (canUpdatePipeline(existingId, request.streamDevice))
                {
                    updatePipelineInternal(request.id, request.streamDevice);
                }
                break;
            }
            case PipelineOperation::Run:
            {
                PipelineID  existingId;
                if (findMatchingPipeline(request.streamDevice, existingId))
                {
                    MX_LOG_INFO("PipelineManager", ("match same pipeline so can't create new just starting ID :" + std::to_string(existingId)).c_str());
                    // TODO : what to do blindlly start 
                    startPipeline(request.id);

                    break;
                }
                createPipelineInternal(request.id, request.streamDevice);
                startPipeline(request.id);
                break;
            }
            case PipelineOperation::Start:
            {
                startPipeline(request.id);
                break;
            }
            case PipelineOperation::Stop:
            {
                stopPipeline(request.id);
                break;
            }
            case PipelineOperation::Pause:
            {
                pausePipeline(request.id);
                break;
            }
            case PipelineOperation::Resume:
            {
                resumePipeline(request.id);
                break;
            }
            case PipelineOperation::Terminate:
            {
                terminatePipeline(request.id);
                break;
            }
            default :
            break;
            }
        }
        catch (const std::exception& e)
        {
            MX_LOG_ERROR("PipelineHandler", ("failed to create pipeline:" + request.streamDevice.name() + e.what()).c_str());
        }
    }

    m_running = false;
}


///////////////////////////////////////////////    Control operations  //////////////////////////////////////////

void PipelineManager::createPipelineInternal(PipelineID id, const MediaStreamDevice& streamDevice)
{
    std::lock_guard<std::mutex> lock(m_mutex);

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
            MX_LOG_INFO("PipelineManager", ("Pipeline " + std::to_string(id) + " state changed to: " + stateStr).c_str());
        });

    handler->setErrorCallback([this, id](const std::string& error)
        {
            MX_LOG_ERROR("PipelineManager", ("Pipeline " + std::to_string(id) + " error " + error).c_str());
        });

    m_pipelineHandlers[id] = std::move(handler);

    MX_LOG_TRACE("PipelineManager", ("Created pipeline with ID" + std::to_string(id)).c_str());
}

void PipelineManager::updatePipelineInternal(PipelineID id, const MediaStreamDevice& streamDevice)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_pipelineHandlers.find(id);
    if (it != m_pipelineHandlers.end())
    {
        if (it->second->updateConfiguration(streamDevice))
        {
            MX_LOG_TRACE("PipelineManager", ("Successfully updated pipeline: " + std::to_string(id)).c_str());
        }
        else
        {
            MX_LOG_ERROR("PipelineManager", ("Failed to update pipeline: " + std::to_string(id)).c_str());
        }
    }
}

bool PipelineManager::startPipeline(PipelineID id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end())
    {
        MX_LOG_TRACE("PipelineManager", ("start Pipeline: " + std::to_string(id)).c_str());
        return it->second->start();
    }
    return false;
}

bool PipelineManager::pausePipeline(PipelineID id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end())
    {
        MX_LOG_TRACE("PipelineManager", ("pause Pipeline: " + std::to_string(id)).c_str());
        return it->second->pause();
    }
    return false;
}

bool PipelineManager::resumePipeline(PipelineID id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end())
    {
        MX_LOG_TRACE("PipelineManager", ("resume Pipeline: " + std::to_string(id)).c_str());
        return it->second->resume();
    }
    return false;
}

bool PipelineManager::stopPipeline(PipelineID id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end())
    {
        MX_LOG_TRACE("PipelineManager", ("stop Pipeline: " + std::to_string(id)).c_str());
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
        MX_LOG_TRACE("PipelineManager", ("terminate Pipeline: " + std::to_string(id)).c_str());
        it->second->terminate();
        m_pipelineHandlers.erase(it);
        return true;
    }
    return false;
}



///////////////////////////////////////////////   Pipeline Status queries  //////////////////////////////////////////
bool PipelineManager::isPipelineRunning(PipelineID id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end())
    {
        MX_LOG_TRACE("PipelineManager", ("Pipeline is running ID: " + std::to_string(id)).c_str());
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
    return m_pipelinerequest.size();
}
