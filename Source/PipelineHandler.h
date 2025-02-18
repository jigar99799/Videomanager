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

// Forward declaration
struct MediaStreamDevice;

class PipelineHandler {
public:
    enum class State {
        INITIAL,
        READY,
        PLAYING,
        PAUSED,
        STOPPED,
        ERROR
    };

    using StateCallback = std::function<void(State)>;
    using ErrorCallback = std::function<void(const std::string&)>;

private:
    struct GstElements {
        using GstDeleter = void(*)(GstElement*);
        std::unique_ptr<GstElement, GstDeleter> pipeline;
        std::unique_ptr<GstElement, GstDeleter> source;
        GstElement* depay{nullptr};
        GstElement* parser{nullptr};
        GstElement* decoder{nullptr};
        GstElement* convert{nullptr};
        GstElement* glupload{nullptr};
        GstElement* glcolorconvert{nullptr};
        GstElement* sink{nullptr};
        
        GstElements() : 
            pipeline(nullptr, reinterpret_cast<GstDeleter>(gst_object_unref)),
            source(nullptr, reinterpret_cast<GstDeleter>(gst_object_unref)) {}
    };

    // Pipeline elements and configuration
    GstElements elements;
    MediaStreamDevice config;
    std::atomic<State> currentState{State::INITIAL};
    
    // Synchronization
    mutable std::mutex mtx;
    
    // Callbacks
    StateCallback stateCallback;
    ErrorCallback errorCallback;
    
    // Pipeline building methods
    bool buildPipeline();
    bool configurePipeline();
    void cleanupPipeline();
    
    // Dynamic pad handling
    static void onPadAdded(GstElement* element, GstPad* pad, gpointer data);
    bool linkDynamicPad(GstPad* pad);
    
    // Error handling
    void handleError(const std::string& error);
    static gboolean busCallback(GstBus* bus, GstMessage* msg, gpointer data);

    void handleEndOfStream() {
        if (stateCallback) {
            stateCallback(State::STOPPED);
        }
    }
    
    void handleStateChange(GstState new_state) {
        State state;
        switch (new_state) {
            case GST_STATE_PLAYING: state = State::PLAYING; break;
            case GST_STATE_PAUSED: state = State::PAUSED; break;
            case GST_STATE_READY: state = State::READY; break;
            case GST_STATE_NULL: state = State::STOPPED; break;
            default: state = State::ERROR; break;
        }
        if (stateCallback) {
            stateCallback(state);
        }
    }

    static void padAddedHandlerStatic(GstElement* src, GstPad* new_pad, gpointer data);
    void padAddedHandler(GstElement* src, GstPad* new_pad);

public:
    explicit PipelineHandler(const MediaStreamDevice& streamDevice);
    ~PipelineHandler();

    // Delete copy and move operations
    PipelineHandler(const PipelineHandler&) = delete;
    PipelineHandler& operator=(const PipelineHandler&) = delete;
    PipelineHandler(PipelineHandler&&) = delete;
    PipelineHandler& operator=(PipelineHandler&&) = delete;

    // Pipeline control
    bool start();
    bool pause();
    bool resume();
    bool stop();
    void terminate();
    
    // Configuration
    bool updateConfiguration(const MediaStreamDevice& newConfig);
    const MediaStreamDevice& getConfig() const { return config; }
    
    // Status
    bool isRunning() const;
    State getState() const;
    
    // Callbacks
    void setStateCallback(StateCallback callback);
    void setErrorCallback(ErrorCallback callback);
};

#endif  // PIPELINE_HANDLER_H
