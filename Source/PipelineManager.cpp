#include "PipelineManager.h"
#include "MediaStreamDevice.h"
#include <iostream>
#include <chrono>
#include <random>
#include <sstream>
#include <atomic>
#include "mx_logger.h"

// Constructor
PipelineManager::PipelineManager() 
{
   // for time being no need to init here 
}

// Destructor
PipelineManager::~PipelineManager() 
{
    MX_LOG_TRACE("PipelineManager", "pipeline process shutdown start");
    stopworkerthread();

    // Clear all pipelines
    std::lock_guard<std::mutex> lock(m_pipemangermutex);
    m_pipelineHandlers.clear();
}

///////////////////////////////////////////////    Initialization   ///////////////////////////////////////////////

void PipelineManager::initializemanager()
{
    gst_init(nullptr, nullptr);
    startworkerthread();
}

void PipelineManager::sendPipelineRequest(PipelineRequest incommingrequest)
{
    enqueuePipelineRequest(incommingrequest);
}

///////////////////////////////////////////////    Pipeline comparison   //////////////////////////////////////////

bool PipelineManager::findMatchingpipeline(const MediaStreamDevice& streamDevice, PipelineID& existingId)
{
    // TODO : check all data ?? 
    std::lock_guard<std::mutex> lock(m_pipemangermutex);
    
    for (const auto& [id, handler] : m_pipelineHandlers)
    {
        const MediaStreamDevice& existingConfig = handler->getConfig();
        
        // Compare the essential properties
        if (existingConfig == streamDevice)
        {
            existingId = id;
            MX_LOG_TRACE("PipelineManager", ("matching pipeline found ID: " + std::to_string(existingId)).c_str());
            return true;
        }
    }
    
    MX_LOG_TRACE("PipelineManager", ("matching pipeline not found for devide : " + streamDevice.name()).c_str());
    return false;
}

bool PipelineManager::validatepipelineConfig(const MediaStreamDevice& streamDevice) const
{
    // TODO : Validate source type and network configuration or more ??
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

bool PipelineManager::ispipelineexists(PipelineID id)
{
    std::lock_guard<std::mutex> lock(m_pipemangermutex);
    return m_pipelineHandlers.find(id) != m_pipelineHandlers.end();
}

bool PipelineManager::canUpdatePipeline(PipelineID id, const MediaStreamDevice& streamDevice)
{
    // TODO  : what to allow and what not to
    std::lock_guard<std::mutex> lock(m_pipemangermutex);

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
        std::lock_guard<std::mutex> lock(m_pipemangermutex);
        m_pipelinerequest.enqueue(request);
    }
    m_cv.notify_one();
    
    // TODO : Does this need to inform ? 
    // Notify that request was received
    onHandlerCallback(
        PipelineStatus::InProgress,
        request.getPipelineID(),
        request.getRequestID(),
        "Request received and enqueue by Pipeline Manager");
}

void PipelineManager::processpipelinerequest()
{
    m_running = true;

    while (m_running)
    {
        PipelineRequest request;
        {
            std::unique_lock<std::mutex> lock(m_pipemangermutex);
            m_cv.wait(lock, [this]
                {
                    return !m_running || !m_pipelinerequest.isEmpty();
                });

            if (!m_running) break;

            request = m_pipelinerequest.dequeue();
        }

        if (request.getEAction() == eAction::ACTION_CREATE ||
            request.getEAction() == eAction::ACTION_UPDATE ||
            request.getEAction() == eAction::ACTION_START)
        {
            if (!validatepipelineConfig(request.getMediaStreamDevice()))
            {
                MX_LOG_ERROR("PipelineManager", ("invalid configuration received for device :" + request.getMediaStreamDevice().name()).c_str());
                continue;
            }
        }

        try
        {
            switch (request.getEAction())
            {
            case eAction::ACTION_CREATE:
            {
                PipelineID  existingId;
                if (findMatchingpipeline(request.getMediaStreamDevice(), existingId))
                {
                    MX_LOG_INFO("PipelineManager", ("match same pipeline so can't created :" + std::to_string(existingId)).c_str());
                    // TODO : what to do blindlly created or return 
                    break;
                }
                createPipelineInternal(request.getPipelineID(), request.getRequestID(), request.getMediaStreamDevice());
                break;
            }
            case eAction::ACTION_UPDATE:
            {
                PipelineID  existingId;
                if (!findMatchingpipeline(request.getMediaStreamDevice(), existingId))
                {
                    MX_LOG_INFO("PipelineManager", ("match same pipeline not found while updatating for device :" + request.getMediaStreamDevice().name()).c_str());
                    // TODO : what to do blindlly created or return 
                    break;
                }

                if (canUpdatePipeline(existingId, request.getMediaStreamDevice()))
                {
                    updatePipelineInternal(request.getPipelineID(), request.getMediaStreamDevice());
                }
                break;
            }
            case eAction::ACTION_RUN:
            {
                PipelineID  existingId;
                if (findMatchingpipeline(request.getMediaStreamDevice(), existingId))
                {
                    MX_LOG_INFO("PipelineManager", ("match same pipeline so can't create new just starting ID :" + std::to_string(existingId)).c_str());
                    // TODO : what to do blindlly start 
                    startPipeline(request.getPipelineID());

                    break;
                }
                createPipelineInternal(request.getPipelineID(), request.getRequestID(), request.getMediaStreamDevice());
                startPipeline(request.getPipelineID());
                break;
            }
            case eAction::ACTION_START:
            {
                PipelineID  existingId;
                if (!findMatchingpipeline(request.getMediaStreamDevice(), existingId))
                {
                    MX_LOG_INFO("PipelineManager", ("match same pipeline so can create first and then start  :" + std::to_string(existingId)).c_str());
                    createPipelineInternal(request.getPipelineID(), request.getRequestID(), request.getMediaStreamDevice());
                }
                startPipeline(request.getPipelineID());
                break;
            }
            case eAction::ACTION_STOP:
            {
                stopPipeline(request.getPipelineID());
                break;
            }
            case eAction::ACTION_PAUSE:
            {
                pausePipeline(request.getPipelineID());
                break;
            }
            case eAction::ACTION_RESUME:
            {
                resumePipeline(request.getPipelineID());
                break;
            }
            case eAction::ACTION_TERMINATE:
            {
                terminatePipeline(request.getPipelineID());
                break;
            }
            default :
            break;
            }
        }
        catch (const std::exception& e)
        {
            MX_LOG_ERROR("PipelineHandler", ("failed to create pipeline:" + request.getMediaStreamDevice().name() + e.what()).c_str());
        }
    }

    m_running = false;
}


///////////////////////////////////////////////    Control operations  //////////////////////////////////////////

void PipelineManager::createPipelineInternal(PipelineID id, size_t iRequestID, const MediaStreamDevice& streamDevice)
{
    std::lock_guard<std::mutex> lock(m_pipemangermutex);

    try 
    {
        // Create a new pipeline handler
        auto handler = std::make_unique<PipelineHandler>(streamDevice);

        // Set the pipeline ID
        handler->setPipelineId(id);
        handler->setCurrentRequestId(iRequestID);

        // Set unified callback instead of separate state and error callbacks
        handler->setCallback(
            std::bind(&PipelineManager::onHandlerCallback, 
                     this,
                     std::placeholders::_1,
                     std::placeholders::_2,
                     std::placeholders::_3,
                     std::placeholders::_4));

        // Store the handler
        m_pipelineHandlers[id] = std::move(handler);

        MX_LOG_TRACE("PipelineManager", ("Created pipeline with ID: " + std::to_string(id)).c_str());
    } 
    catch (const std::exception& e) 
    {
        onHandlerCallback(
            PipelineStatus::Error,
            id,
            iRequestID,
            std::string("Failed to create pipeline: ") + e.what()
        );
        throw; // Re-throw to be caught by the caller
    }
}

void PipelineManager::updatePipelineInternal(PipelineID id, const MediaStreamDevice& streamDevice)
{
    std::lock_guard<std::mutex> lock(m_pipemangermutex);

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
    std::lock_guard<std::mutex> lock(m_pipemangermutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end())
    {
        MX_LOG_TRACE("PipelineManager", ("start Pipeline: " + std::to_string(id)).c_str());
        return it->second->start();
    }
    return false;
}

bool PipelineManager::pausePipeline(PipelineID id)
{
    std::lock_guard<std::mutex> lock(m_pipemangermutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end())
    {
        MX_LOG_TRACE("PipelineManager", ("pause Pipeline: " + std::to_string(id)).c_str());
        return it->second->pause();
    }
    return false;
}

bool PipelineManager::resumePipeline(PipelineID id)
{
    std::lock_guard<std::mutex> lock(m_pipemangermutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end())
    {
        MX_LOG_TRACE("PipelineManager", ("resume Pipeline: " + std::to_string(id)).c_str());
        return it->second->resume();
    }
    return false;
}

bool PipelineManager::stopPipeline(PipelineID id)
{
    std::lock_guard<std::mutex> lock(m_pipemangermutex);
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
    std::lock_guard<std::mutex> lock(m_pipemangermutex);
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
bool PipelineManager::isPipelineRunning(PipelineID id)
{
    std::lock_guard<std::mutex> lock(m_pipemangermutex);
    if (auto it = m_pipelineHandlers.find(id); it != m_pipelineHandlers.end())
    {
        MX_LOG_TRACE("PipelineManager", ("Pipeline is running ID: " + std::to_string(id)).c_str());
        return it->second->isRunning();
    }
    return false;
}

std::vector<PipelineID> PipelineManager::getActivePipelines()
{
    std::lock_guard<std::mutex> lock(m_pipemangermutex);
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

size_t PipelineManager::getQueueSize()
{
    std::lock_guard<std::mutex> lock(m_pipemangermutex);
    return m_pipelinerequest.size();
}

void PipelineManager::onHandlerCallback(PipelineStatus status, size_t pipelineId,
                         size_t requestId, const std::string& message)
{
    // Manager can add additional context here if needed
    if (m_callback)
    {
        std::cerr << "onHandlerCallback sdfsdfsdfsdf " << message << std::endl;

        // Instead of calling directly, use the callback which will now queue it
        m_callback(status, pipelineId, requestId, message);
    }
}