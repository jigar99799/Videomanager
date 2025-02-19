#include "PipelineHandler.h"
#include <iostream>
#include <sstream>

PipelineHandler::PipelineHandler(const MediaStreamDevice& streamDevice)
    : config(streamDevice) 
{
    gst_init(nullptr, nullptr);

    if (!buildPipeline()) 
    {
        handleError("Failed to build pipeline");
    }
}

PipelineHandler::~PipelineHandler() 
{
    terminate();
}

bool PipelineHandler::buildPipeline() 
{
    if (!configurePipeline()) 
    {
        return false;
    }
    return true;
}

bool PipelineHandler::configurePipeline() 
{
    MediaStreamDevice device = config;
    
    try
    {
        // Get input and output configurations from PipelineManager
        MediaData inputData = device.stinputMediaData;
        MediaData outputData = device.stoutputMediaData;

        elements.pipeline = gst_pipeline_new(device.sDeviceName.c_str());

        // Step 1: Identify Source Element
        if (inputData.esourceType == eSourceType::SOURCE_TYPE_FILE)
        {
            elements.source = gst_element_factory_make("filesrc", "source");
            g_object_set(G_OBJECT(elements.source), "location", device.sDeviceName.c_str(), NULL);
        }
        else if (inputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
        {
            elements.source = gst_element_factory_make("rtspsrc", "source");
            g_object_set(G_OBJECT(elements.source), "location", device.sDeviceName.c_str(), NULL);
            g_signal_connect(elements.source, "pad-added", G_CALLBACK(on_pad_added), this);
        }
        else
        {
            MX_LOG_ERROR("PipelineHandler", "Unsupported source type found while configured");
            return false;
        }


        // Step 2: Add Depayloader for RTP/RTSP sources
        if (inputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
        {
            if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
            {
                elements.depay = gst_element_factory_make("rtph264depay", "depay");
            }
            else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
            {
                elements.depay = gst_element_factory_make("rtph265depay", "depay");
            }
        }

        // Step 3: Add Demuxer if Input is a Container Format
        if (inputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MP4 ||
            inputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MKV)
        {
            if (inputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MP4)
            {
                elements.demuxer = gst_element_factory_make("qtdemux", "demuxer");
            }
            else if (inputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MKV)
            {
                elements.demuxer = gst_element_factory_make("matroskademux", "demuxer");
            }
        }


        // Step 4: Add Parser for Compressed Formats
        if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
        {
            elements.parser = gst_element_factory_make("h264parse", "parser");
        }
        else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
        {
            elements.parser = gst_element_factory_make("h265parse", "parser");
        }


        // Step 5: Add Decoder
        if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
        {
            elements.decoder = gst_element_factory_make("avdec_h264", "decoder");
        }
        else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
        {
            elements.decoder = gst_element_factory_make("avdec_h265", "decoder");
        }


        // Step 6: Add Video Converter
        elements.convert = gst_element_factory_make("videoconvert", "convert");


        // Step 7: Add Encoder (If Required)
        if (outputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
        {
            elements.encoder = gst_element_factory_make("x264enc", "encoder");
        }
        else if (outputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
        {
            elements.encoder = gst_element_factory_make("x265enc", "encoder");
        }


        // Step 8: Add Muxer if Output is a Container Format
        if (outputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MP4)
        {
            elements.muxer = gst_element_factory_make("mp4mux", "muxer");
        }
        else if (outputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MKV)
        {
            elements.muxer = gst_element_factory_make("matroskamux", "muxer");
        }


        // Step 9: Add Payloader for Network Streaming
        // if (outputData.stNetworkStreaming == StreamProtocol::RTP) 
        // {
        //     payloader = gst_element_factory_make("rtph264pay", "payloader");
        // }

        // Step 10: Identify Sink Element
        if (outputData.esourceType == eSourceType::SOURCE_TYPE_FILE)
        {
            elements.sink = gst_element_factory_make("filesink", "sink");
        }
        else if (outputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
        {
            elements.sink = gst_element_factory_make("udpsink", "sink");
        }
        else
        {
            elements.sink = gst_element_factory_make("autovideosink", "sink");
        }

        // Step 11: Add Elements to the Pipeline
        gst_bin_add_many(GST_BIN(elements.pipeline), elements.source, NULL);

        if (elements.depay) gst_bin_add(GST_BIN(elements.pipeline), elements.depay);
        if (elements.demuxer) gst_bin_add(GST_BIN(elements.pipeline), elements.demuxer);
        if (elements.parser) gst_bin_add(GST_BIN(elements.pipeline), elements.parser);
        if (elements.decoder) gst_bin_add(GST_BIN(elements.pipeline), elements.decoder);
        if (elements.convert) gst_bin_add(GST_BIN(elements.pipeline), elements.convert);
        if (elements.encoder) gst_bin_add(GST_BIN(elements.pipeline), elements.encoder);
        if (elements.muxer) gst_bin_add(GST_BIN(elements.pipeline), elements.muxer);
        if (elements.payloader) gst_bin_add(GST_BIN(elements.pipeline), elements.payloader);
        if (elements.sink) gst_bin_add(GST_BIN(elements.pipeline), elements.sink);

        // Step 12: Link Elements
        if (inputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK) {
            gst_element_link_many(elements.source, elements.depay, elements.decoder, elements.convert, NULL);
        }
        else if (inputData.esourceType == eSourceType::SOURCE_TYPE_FILE) {
            gst_element_link_many(elements.source, elements.demuxer, NULL);
            g_signal_connect(elements.demuxer, "pad-added", G_CALLBACK(on_pad_added), this);
        }

        gst_bin_add_many(GST_BIN(elements.pipeline), elements.source, elements.depay, elements.decoder, elements.convert, elements.sink, NULL);
        gst_element_link_many(elements.depay, elements.decoder, elements.convert, elements.sink, NULL);

        MX_LOG_INFO("PipelineHandler", ("pipe line is sucessfully build " + device.sDeviceName).c_str());

    }
    catch (std::exception& e)
    {
        MX_LOG_ERROR("PipelineHandler", ("pipe line is not build" + device.sDeviceName + e.what()).c_str());
        return false;
    }
    return true;
}

void PipelineHandler::cleanupPipeline() 
{
    if (elements.pipeline) 
    {
        gst_object_unref(elements.pipeline);
        elements.reset();  // Reset all elements to nullptr
    }
}

bool PipelineHandler::start() 
{
    std::lock_guard<std::mutex> lock(mtx);
    
    if (currentState == State::PLAYING) 
    {
        return true;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(elements.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) 
    {
        handleError("Failed to start pipeline state is not chaged ");
        return false;
    }

    currentState = State::PLAYING;

    MX_LOG_TRACE("PipelineHandler", "pipeline state set to start");

    if (stateCallback) 
        stateCallback(State::PLAYING);

    return true;
}

bool PipelineHandler::pause() 
{
    std::lock_guard<std::mutex> lock(mtx);
    
    if (currentState != State::PLAYING) 
    {
        return false;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(elements.pipeline, GST_STATE_PAUSED);;
    if (ret == GST_STATE_CHANGE_FAILURE) 
    {
        handleError("Failed to pause pipeline state is not chaged");
        return false;
    }

    currentState = State::PAUSED;

    MX_LOG_TRACE("PipelineHandler", "pipeline state set to paused");

    if (stateCallback) 
        stateCallback(State::PAUSED);

    return true;
}

bool PipelineHandler::resume() 
{
    MX_LOG_TRACE("PipelineHandler", "pipeline resume");
    return start();  // Reuse start logic
}

bool PipelineHandler::stop() 
{
    std::lock_guard<std::mutex> lock(mtx);
    
    if (currentState == State::STOPPED) 
    {
        return true;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(elements.pipeline, GST_STATE_NULL);

    if (ret == GST_STATE_CHANGE_FAILURE) 
    {
        handleError("Failed to stop pipeline state is not chaged ");
        return false;
    }
    
    currentState = State::STOPPED;

    MX_LOG_TRACE("PipelineHandler", "pipeline state set to stopped");

    if (stateCallback) 
        stateCallback(State::STOPPED);

    return true;
}

void PipelineHandler::terminate() 
{
    stop();
    cleanupPipeline();
}

bool PipelineHandler::updateConfiguration(const MediaStreamDevice& newConfig) 
{
    std::lock_guard<std::mutex> lock(mtx);
    
    // Store current state
    State previousState = currentState;
    
    MX_LOG_TRACE("PipelineHandler", "stop pipeline for new configuration update");

    // Stop pipeline
    if (!stop()) 
    {
        MX_LOG_ERROR("PipelineHandler", "failed to stop pipeline");
        return false;
    }
    
    // Update configuration
    config = newConfig;

    MX_LOG_TRACE("PipelineHandler", "new configuration is updated");
    
    // Rebuild pipeline
    cleanupPipeline();

    if (!buildPipeline()) 
    {
        handleError("Failed to rebuild pipeline with new configuration");
        return false;
    }
    
    // Restore previous state if it was playing or paused
    if (previousState == State::PLAYING) 
    {
        return start();
    } 
    else if (previousState == State::PAUSED) 
    {
        return pause();
    }
    
    return true;
}

bool PipelineHandler::isRunning() const 
{
    return currentState == State::PLAYING;
}

PipelineHandler::State PipelineHandler::getState() const 
{
    return currentState;
}

void PipelineHandler::setStateCallback(StateCallback callback) 
{
    std::lock_guard<std::mutex> lock(mtx);
    stateCallback = std::move(callback);
}

void PipelineHandler::setErrorCallback(ErrorCallback callback) 
{
    std::lock_guard<std::mutex> lock(mtx);
    errorCallback = std::move(callback);
}

void PipelineHandler::handleError(const std::string& error) 
{
    currentState = State::ERROR;
    if (errorCallback) 
    {
        errorCallback(error);
    }
    MX_LOG_ERROR("PipelineHandler", ("Pipeline error:" + error).c_str());
}

void PipelineHandler::on_pad_added(GstElement* element, GstPad* pad, gpointer data)
{
    PipelineHandler* self = static_cast<PipelineHandler*>(data);
    self->linkDynamicPad(pad);
}

bool PipelineHandler::linkDynamicPad(GstPad* pad) 
{
    GstCaps* caps = gst_pad_get_current_caps(pad);

    if (!caps)
    {
        MX_LOG_ERROR("PipelineHandler", "no padadded due to caps not found");
        return false;
    }

    GstPad* sinkPad = gst_element_get_static_pad(elements.depay, "sink");

    if (!sinkPad || gst_pad_link(pad, sinkPad))
    {
        gst_object_unref(sinkPad);
    }

    if (gst_pad_link(pad, sinkPad) != GST_PAD_LINK_OK)
    {
        MX_LOG_ERROR("PipelineHandler", "failed to link dynamic pad");
    }
    else
        MX_LOG_ERROR("PipelineHandler", "successfully to link dynamic pad");
    
    gst_caps_unref(caps);
    gst_object_unref(sinkPad);
    return true;
}

gboolean PipelineHandler::busCallback(GstBus* bus, GstMessage* msg, gpointer data)
{
    PipelineHandler* handler = static_cast<PipelineHandler*>(data);

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
    {
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
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(handler->elements.pipeline))
        {
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
