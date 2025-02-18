#ifndef PIPELINE_H
#define PIPELINE_H

#include "Struct.h"  

class Pipeline {
public:
    size_t m_uiPipelineID;
    size_t m_uiRequestID;
    eAction m_eAction;
    MediaStreamDevice m_stMediaStreamDevice;
    char arr[2*1024*1024];

public:
    // Default constructor
    Pipeline();

    // Parameterized constructor
    Pipeline(size_t pipelineID, size_t requestID, eAction action, const MediaStreamDevice& mediaStreamDevice);

    // Copy constructor
    Pipeline(const Pipeline& other);

    // Assignment operator
    Pipeline& operator=(const Pipeline& other);

    // Destructor
    ~Pipeline();

    // Getter and Setter functions
    inline size_t getPipelineID() const { return m_uiPipelineID; }
    inline void setPipelineID(size_t pipelineID) { m_uiPipelineID = pipelineID; }

    inline size_t getRequestID() const { return m_uiRequestID; }
    inline void setRequestID(size_t requestID) { m_uiRequestID = requestID; }

    inline eAction getEAction() const { return m_eAction; }
    inline void setEAction(eAction action) { m_eAction = action; }

    inline MediaStreamDevice getMediaStreamDevice() const { return m_stMediaStreamDevice; }
    inline void setMediaStreamDevice(const MediaStreamDevice& mediaStreamDevice) { m_stMediaStreamDevice = mediaStreamDevice; }
};

#endif // PIPELINE_H
