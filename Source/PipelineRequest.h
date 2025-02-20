#ifndef PIPELINEREQUEST_H
#define PIPELINEREQUEST_H

#include "Struct.h"

class PipelineRequest
{
public:
    size_t m_uiPipelineID;
    size_t m_uiRequestID;
    eAction m_eAction;
    MediaStreamDevice m_stMediaStreamDevice;

public:
    // Default constructor
    PipelineRequest();

    // Parameterized constructor
    PipelineRequest(size_t pipelineID, size_t requestID, eAction action, const MediaStreamDevice& mediaStreamDevice);

    // Copy constructor
    PipelineRequest(const PipelineRequest& other);

    // Move constructor
    PipelineRequest(PipelineRequest&& other) noexcept;

    // Assignment operator
    PipelineRequest& operator=(const PipelineRequest& other);

    // Move assignment operator
    PipelineRequest& operator=(PipelineRequest&& other) noexcept;

    // Destructor
    ~PipelineRequest();

    // Getter and Setter functions
    inline size_t getPipelineID() const { return m_uiPipelineID; }
    inline void setPipelineID(size_t pipelineID) { m_uiPipelineID = pipelineID; }

    inline size_t getRequestID() const { return m_uiRequestID; }
    inline void setRequestID(size_t requestID) { m_uiRequestID = requestID; }

    inline eAction getEAction() const { return m_eAction; }
    inline void setEAction(eAction action) { m_eAction = action; }

    inline const MediaStreamDevice& getMediaStreamDevice() const { return m_stMediaStreamDevice; }
    inline void setMediaStreamDevice(const MediaStreamDevice& mediaStreamDevice) { m_stMediaStreamDevice = mediaStreamDevice; }
};

#endif // PIPELINEREQUEST_H
