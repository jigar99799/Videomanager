#include "Pipeline.h"

// Default constructor
Pipeline::Pipeline()
    : m_uiPipelineID(0), m_uiRequestID(0), m_eAction(eAction::ACTION_NONE), m_stMediaStreamDevice() {
}

// Parameterized constructor
Pipeline::Pipeline(size_t pipelineID, size_t requestID, eAction action, const MediaStreamDevice& mediaStreamDevice)
    : m_uiPipelineID(pipelineID), m_uiRequestID(requestID), m_eAction(action), m_stMediaStreamDevice(mediaStreamDevice) {
}

// Copy constructor
Pipeline::Pipeline(const Pipeline& other)
    : m_uiPipelineID(other.m_uiPipelineID),
      m_uiRequestID(other.m_uiRequestID), 
      m_eAction(other.m_eAction),
      m_stMediaStreamDevice(other.m_stMediaStreamDevice) {
}

// Assignment operator
Pipeline& Pipeline::operator=(const Pipeline& other) {
    if (this != &other) { // Prevent self-assignment
        m_uiPipelineID = other.m_uiPipelineID;
        m_uiRequestID = other.m_uiRequestID;
        m_eAction = other.m_eAction;
        m_stMediaStreamDevice = other.m_stMediaStreamDevice;
    }
    return *this;
}

Pipeline::~Pipeline() {
}
