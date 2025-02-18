#include "PipelineHandler.h"
#include <iostream>
#include <sstream>

PipelineHandler::PipelineHandler(const MediaStreamDevice& streamDevice)
    : config(streamDevice) {
    gst_init(nullptr, nullptr);
    if (!buildPipeline()) {
        handleError("Failed to build pipeline");
    }
}

PipelineHandler::~PipelineHandler() {
    terminate();
}

bool PipelineHandler::buildPipeline() {
    if (!configurePipeline()) {
        handleError("Failed to configure pipeline");
        return false;
    }

    // No need to connect to demuxer's pad-added signal anymore since we're using rtspsrc
    // Instead, we're already handling pad-added in configurePipeline()

    // Set pipeline state to PLAYING
    GstStateChangeReturn ret = gst_element_set_state(elements.pipeline.get(), GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        handleError("Failed to set pipeline to playing state");
        return false;
    }

    return true;
}

bool PipelineHandler::configurePipeline() {
    // Create GStreamer pipeline using parse_launch for better performance
    std::string pipeline_str = 
        "rtspsrc name=source latency=0 ! "
        "rtph264depay ! "
        "h264parse ! "
        "avdec_h264 ! "
        "videoconvert ! "
        "glupload ! "
        "glcolorconvert ! "
        "glimagesink sync=false async=false";
        
    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    
    if (error) {
        handleError(std::string("Pipeline creation failed: ") + error->message);
        g_error_free(error);
        return false;
    }

    elements.pipeline.reset(pipeline);
    
    // Get pipeline elements by name
    elements.source.reset(gst_bin_get_by_name(GST_BIN(elements.pipeline.get()), "source"));
    
    // Configure RTSP source
    std::string rtsp_url = "rtsp://" + config.address() + ":" + std::to_string(config.port()) + "/";
    g_object_set(G_OBJECT(elements.source.get()), 
                "location", rtsp_url.c_str(),
                "protocols", 0x00000004, // Force TCP
                nullptr);
    
    return true;
}

void PipelineHandler::cleanupPipeline() {
    if (elements.pipeline) {
        gst_element_set_state(elements.pipeline.get(), GST_STATE_NULL);
        elements = GstElements();  // Reset all elements to nullptr
    }
}

bool PipelineHandler::start() {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (currentState == State::PLAYING) {
        return true;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(elements.pipeline.get(), GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        handleError("Failed to start pipeline");
        return false;
    }
    
    currentState = State::PLAYING;
    if (stateCallback) stateCallback(State::PLAYING);
    return true;
}

bool PipelineHandler::pause() {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (currentState != State::PLAYING) {
        return false;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(elements.pipeline.get(), GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        handleError("Failed to pause pipeline");
        return false;
    }
    
    currentState = State::PAUSED;
    if (stateCallback) stateCallback(State::PAUSED);
    return true;
}

bool PipelineHandler::resume() {
    return start();  // Reuse start logic
}

bool PipelineHandler::stop() {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (currentState == State::STOPPED) {
        return true;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(elements.pipeline.get(), GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        handleError("Failed to stop pipeline");
        return false;
    }
    
    currentState = State::STOPPED;
    if (stateCallback) stateCallback(State::STOPPED);
    return true;
}

void PipelineHandler::terminate() {
    stop();
    cleanupPipeline();
}

bool PipelineHandler::updateConfiguration(const MediaStreamDevice& newConfig) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Store current state
    State previousState = currentState;
    
    // Stop pipeline
    if (!stop()) {
        return false;
    }
    
    // Update configuration
    config = newConfig;
    
    // Rebuild pipeline
    cleanupPipeline();
    if (!buildPipeline()) {
        handleError("Failed to rebuild pipeline with new configuration");
        return false;
    }
    
    // Restore previous state if it was playing or paused
    if (previousState == State::PLAYING) {
        return start();
    } else if (previousState == State::PAUSED) {
        return pause();
    }
    
    return true;
}

bool PipelineHandler::isRunning() const {
    return currentState == State::PLAYING;
}

PipelineHandler::State PipelineHandler::getState() const {
    return currentState;
}

void PipelineHandler::setStateCallback(StateCallback callback) {
    std::lock_guard<std::mutex> lock(mtx);
    stateCallback = std::move(callback);
}

void PipelineHandler::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mtx);
    errorCallback = std::move(callback);
}

void PipelineHandler::handleError(const std::string& error) {
    currentState = State::ERROR;
    if (errorCallback) {
        errorCallback(error);
    }
    std::cerr << "Pipeline error: " << error << std::endl;
}

void PipelineHandler::onPadAdded(GstElement* element, GstPad* pad, gpointer data) {
    PipelineHandler* self = static_cast<PipelineHandler*>(data);
    self->linkDynamicPad(pad);
}

bool PipelineHandler::linkDynamicPad(GstPad* pad) {
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps) return false;
    
    const gchar* str = gst_structure_get_name(gst_caps_get_structure(caps, 0));
    bool success = false;
    
    if (g_str_has_prefix(str, "video/")) {
        GstPad* sinkpad = gst_element_get_static_pad(elements.parser, "sink");
        if (sinkpad) {
            success = (gst_pad_link(pad, sinkpad) == GST_PAD_LINK_OK);
            gst_object_unref(sinkpad);
        }
    }
    
    gst_caps_unref(caps);
    return success;
}

gboolean PipelineHandler::busCallback(GstBus* bus, GstMessage* msg, gpointer data) {
    PipelineHandler* handler = static_cast<PipelineHandler*>(data);
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* err;
            gchar* debug_info;
            gst_message_parse_error(msg, &err, &debug_info);
            handler->handleError(std::string("Pipeline error: ") + err->message);
            g_error_free(err);
            g_free(debug_info);
            break;
        }
        case GST_MESSAGE_EOS:
            handler->handleEndOfStream();
            break;
        case GST_MESSAGE_STATE_CHANGED:
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(handler->elements.pipeline.get())) {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                handler->handleStateChange(new_state);
            }
            break;
        default:
            break;
    }
    
    return TRUE;
}

// Static pad added handler
void PipelineHandler::padAddedHandlerStatic(GstElement* src, GstPad* new_pad, gpointer data) {
    PipelineHandler* handler = static_cast<PipelineHandler*>(data);
    handler->padAddedHandler(src, new_pad);
}

// Member pad added handler
void PipelineHandler::padAddedHandler(GstElement* src, GstPad* new_pad) {
    GstCaps* new_pad_caps = gst_pad_get_current_caps(new_pad);
    GstStructure* str = gst_caps_get_structure(new_pad_caps, 0);
    const gchar* type = gst_structure_get_name(str);

    if (g_str_has_prefix(type, "application/x-rtp")) {
        GstPad* sink_pad = gst_element_get_static_pad(elements.depay, "sink");
        
        if (gst_pad_link(new_pad, sink_pad) != GST_PAD_LINK_OK) {
            handleError("Failed to link pads");
        }
        
        gst_object_unref(sink_pad);
    }
    
    if (new_pad_caps != nullptr) {
        gst_caps_unref(new_pad_caps);
    }
}
