#ifndef PIPELINE_HANDLER_H
#define PIPELINE_HANDLER_H

#include <gst/gst.h>
#include <memory>
#include <mutex>
#include <string>
#include <atomic>
#include <functional>
#include <unordered_map>
#include "MediaStreamDevice.h"
#include "PipelineProcess.h" // For PipelineStatus enum
#include "PipelineHandler.h"

// Forward declaration
struct MediaStreamDevice;

// New unified callback
using HandlerCallback = std::function<void(
    PipelineStatus status,
    size_t pipelineId,
    size_t requestId,
    const std::string& message
    )>;

class PipelineHandler 
{
public:
    enum class State 
    {
        INITIAL,
        READY,
        PLAYING,
        PAUSED,
        STOPPED,
        ERROR
    };

    

private:

    struct GstElements
    {
        GstElement* pipeline{ nullptr };
        GstElement* source{ nullptr };
        GstElement* depay{ nullptr };
        GstElement* demuxer{ nullptr };
        GstElement* parser{ nullptr };
        GstElement* decoder{ nullptr };
        GstElement* muxer{ nullptr };
        GstElement* payloader{ nullptr };
        GstElement* encoder{ nullptr };
        GstElement* convert{ nullptr };
        GstElement* sink{ nullptr };

        void reset()
        {
            pipeline = nullptr;
            source = nullptr;
            depay = nullptr;
            demuxer = nullptr;
            parser = nullptr;
            decoder = nullptr;
            muxer = nullptr;
            payloader = nullptr;
            encoder = nullptr;
            convert = nullptr;
            sink = nullptr;

        }
    };

    // Pipeline state
    State m_state;
    std::atomic<bool> m_isRunning{false};
    size_t m_pipelineId{0};
    size_t m_currentRequestId{0};
    
    // Pipeline elements and configuration
    GstElements elements;
    MediaStreamDevice config;

    // Callbacks
    HandlerCallback m_callback{nullptr};
    
    // Mutex for thread safety
    std::mutex m_mutex;
    
    // Pipeline building
    bool buildPipeline();
    bool configurePipeline();
    void cleanupPipeline();
    
    // Error handling
    void handleError(const std::string& error);

   
    // GStreamer callbacks
    static void on_pad_added(GstElement* element, GstPad* pad, gpointer data);
    bool linkDynamicPad(GstPad* pad);
    static gboolean busCallback(GstBus* bus, GstMessage* msg, gpointer data);
    static void padAddedHandlerStatic(GstElement* src, GstPad* new_pad, gpointer data);
    void padAddedHandler(GstElement* src, GstPad* new_pad);

public:
    explicit PipelineHandler(const MediaStreamDevice& streamDevice = MediaStreamDevice());
    ~PipelineHandler();
    
    // Pipeline control
    bool start();
    bool pause();
    bool resume();
    bool stop();
    void terminate();
    
    // Configuration
    bool updateConfiguration(const MediaStreamDevice& newConfig);
    
    // Status
    bool isRunning() const;
    State getState() const;
    
    // Set pipeline ID and request ID
    void setPipelineId(size_t id) { m_pipelineId = id; }
    void setCurrentRequestId(size_t id) { m_currentRequestId = id; }
    
    // Configuration access
    const MediaStreamDevice& getConfig() const { return config; }
    
    // Unified callback
    void setCallback(HandlerCallback callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callback = callback;
    }
    
    // Report status using the callback
    void reportStatus(PipelineStatus status, const std::string& message) 
    {
        if (m_callback) 
        {
            m_callback(status, m_pipelineId, m_currentRequestId, message);
        }
    }

    void handleEndOfStream()
    {
        reportStatus(PipelineStatus::EndofStreamReceived,"End of stream received");
    }

};

#endif  // PIPELINE_HANDLER_H
