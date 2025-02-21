#include "PipelineRequest.h"

// Default constructor
PipelineRequest::PipelineRequest()
    : m_uiPipelineID(0), m_uiRequestID(0), m_eAction(eAction::ACTION_NONE), m_stMediaStreamDevice() 
{
}

// Parameterized constructor
PipelineRequest::PipelineRequest(size_t pipelineID, size_t requestID, eAction action, const MediaStreamDevice& mediaStreamDevice)
    : m_uiPipelineID(pipelineID), m_uiRequestID(requestID), m_eAction(action), m_stMediaStreamDevice(mediaStreamDevice) 
{

}
 
// Copy constructor
PipelineRequest::PipelineRequest(const PipelineRequest& other)
    : m_uiPipelineID(other.m_uiPipelineID),
    m_uiRequestID(other.m_uiRequestID),
    m_eAction(other.m_eAction),
    m_stMediaStreamDevice(other.m_stMediaStreamDevice) 
{
    
}

// Move constructor
PipelineRequest::PipelineRequest(PipelineRequest&& other) noexcept
    : m_uiPipelineID(other.m_uiPipelineID),
    m_uiRequestID(other.m_uiRequestID),
    m_eAction(other.m_eAction),
    m_stMediaStreamDevice(std::move(other.m_stMediaStreamDevice)) 
{
   
}

// Assignment operator
PipelineRequest& PipelineRequest::operator=(const PipelineRequest& other) 
{
    if (this != &other) 
    { 
        // Prevent self-assignment
        m_uiPipelineID = other.m_uiPipelineID;
        m_uiRequestID = other.m_uiRequestID;
        m_eAction = other.m_eAction;
        m_stMediaStreamDevice = other.m_stMediaStreamDevice;
    }
    return *this;
}

// Move assignment operator
PipelineRequest& PipelineRequest::operator=(PipelineRequest&& other) noexcept 
{
    if (this != &other) 
    { 
        // Prevent self-assignment
        m_uiPipelineID = other.m_uiPipelineID;
        m_uiRequestID = other.m_uiRequestID;
        m_eAction = other.m_eAction;
        m_stMediaStreamDevice = std::move(other.m_stMediaStreamDevice);
    }
    return *this;
}

// Destructor
PipelineRequest::~PipelineRequest() 
{
}
