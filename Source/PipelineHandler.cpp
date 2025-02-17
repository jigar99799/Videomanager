#include "PipelineHandler.h"
#include <iostream>

PipelineHandler::PipelineHandler(const Pipeline& pipelineData) : pipelineConfig(pipelineData) {
    buildPipeline();
}

PipelineHandler::~PipelineHandler() {
    stop();
    gst_object_unref(pipeline);
}

void PipelineHandler::on_pad_added(GstElement* src, GstPad* newPad, gpointer data) {
    PipelineHandler* handler = (PipelineHandler*)data;
    handler->linkDynamicPad(newPad);
}

void PipelineHandler::linkDynamicPad(GstPad* newPad) {
    GstPad* sinkPad = gst_element_get_static_pad(depay, "sink");
    if (!sinkPad || gst_pad_is_linked(sinkPad)) {
        gst_object_unref(sinkPad);
        return;
    }
    if (gst_pad_link(newPad, sinkPad) != GST_PAD_LINK_OK) {
        std::cerr << "Failed to link dynamic pad\n";
    }
    else
    std::cerr << "successfully to link dynamic pad\n";
    gst_object_unref(sinkPad);
}
#define CHECK_ELEMENT(el, name) \
    if (!el) { \
        std::cerr << "[ERROR] Failed to create element: " << name << std::endl; \
        return; \
    }
void PipelineHandler::buildPipeline() 
{ 

    gst_init(nullptr,nullptr);
    #if 0
    std::cerr << " buildpipeline api call\n";
    pipeline = gst_pipeline_new("pipeline");

    MediaStreamDevice device = pipelineConfig.getMediaStreamDevice();
    MediaData inputData = device.stinputMediaData;
    MediaData outputData = device.stoutputMediaData;

    source = gst_element_factory_make("rtspsrc", "source");
    g_object_set(G_OBJECT(source), "location", device.sDeviceName.c_str(), "protocols", 0x00000004, NULL);
    g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added), this);

    if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264) {
        depay = gst_element_factory_make("rtph264depay", "depay");
        decoder = gst_element_factory_make("avdec_h264", "decoder");
    } else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265) {
        depay = gst_element_factory_make("rtph265depay", "depay");
        decoder = gst_element_factory_make("avdec_h265", "decoder");
    }

    convert = gst_element_factory_make("videoconvert", "convert");
    sink = gst_element_factory_make("autovideosink", "sink");

    gst_bin_add_many(GST_BIN(pipeline), source, depay, decoder, convert, sink, NULL);
    gst_element_link_many(depay, decoder, convert, sink, NULL);
    #endif
    #if 1
    std::cerr << " buildpipeline api call\n";
    pipeline = gst_pipeline_new("pipeline");


    // Get input and output configurations from PipelineManager
    MediaStreamDevice device = pipelineConfig.getMediaStreamDevice();
    MediaData inputData = device.stinputMediaData;
    MediaData outputData = device.stoutputMediaData;

    // Step 1: Identify Source Element
    if (inputData.esourceType == eSourceType::SOURCE_TYPE_FILE) 
    {
        source = gst_element_factory_make("filesrc", "source");
        g_object_set(G_OBJECT(source), "location", device.sDeviceName.c_str(), NULL);
    } 
    else if (inputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK) 
    {
        source = gst_element_factory_make("rtspsrc", "source");
        g_object_set(G_OBJECT(source), "location", device.sDeviceName.c_str(), NULL);
        g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added), this);
    } 
    else 
    {
        std::cerr << "Unsupported source type\n";
        return;
    }

    // Step 2: Add Depayloader for RTP/RTSP sources
    if (inputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
    {
        if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
        {
            depay = gst_element_factory_make("rtph264depay", "depay");
        } 
        else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
        {
            depay = gst_element_factory_make("rtph265depay", "depay");
        }
    }

    // Step 3: Add Demuxer if Input is a Container Format
    if (inputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MP4 || 
        inputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MKV) 
    {
        if (inputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MP4)
        {
            demuxer = gst_element_factory_make("qtdemux", "demuxer");
        } else if (inputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MKV)
        {
            demuxer = gst_element_factory_make("matroskademux", "demuxer");
        }
    }

    // Step 4: Add Parser for Compressed Formats
    if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
    {
        parser = gst_element_factory_make("h264parse", "parser");
    }
    else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
    {
        parser = gst_element_factory_make("h265parse", "parser");
    }
    
     // Step 5: Add Decoder
    if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
    {
        decoder = gst_element_factory_make("avdec_h264", "decoder");
    }
    else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
    {
        decoder = gst_element_factory_make("avdec_h265", "decoder");
    }

    // Step 6: Add Video Converter
    convert = gst_element_factory_make("videoconvert", "convert");

    // Step 7: Add Encoder (If Required)
    if (outputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264) 
    {
        encoder = gst_element_factory_make("x264enc", "encoder");
    }
    else if (outputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
    {
        encoder = gst_element_factory_make("x265enc", "encoder");
    }

    // Step 8: Add Muxer if Output is a Container Format
    if (outputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MP4)
    {
        muxer = gst_element_factory_make("mp4mux", "muxer");
    } else if (outputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MKV)
    {
        muxer = gst_element_factory_make("matroskamux", "muxer");
    }

    // // Step 9: Add Payloader for Network Streaming
    // if (outputData.stNetworkStreaming == StreamProtocol::RTP) {
    //     payloader = gst_element_factory_make("rtph264pay", "payloader");
    // }

    // Step 10: Identify Sink Element
    if (outputData.esourceType == eSourceType::SOURCE_TYPE_FILE)
    {
        sink = gst_element_factory_make("filesink", "sink");
      //  g_object_set(G_OBJECT(sink), "location", outputData.sFilePath.c_str(), NULL);
    }
    else if (outputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
    {
        sink = gst_element_factory_make("udpsink", "sink");
       // g_object_set(G_OBJECT(sink), "host", outputData.sIPAddress.c_str(), "port", outputData.iPort, NULL);
    } 
    else {
        sink = gst_element_factory_make("autovideosink", "sink");
    }

    // Step 11: Add Elements to the Pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, NULL);
    
    if (depay) gst_bin_add(GST_BIN(pipeline), depay);
    if (demuxer) gst_bin_add(GST_BIN(pipeline), demuxer);
    if (parser) gst_bin_add(GST_BIN(pipeline), parser);
    if (decoder) gst_bin_add(GST_BIN(pipeline), decoder);
    if (convert) gst_bin_add(GST_BIN(pipeline), convert);
    if (encoder) gst_bin_add(GST_BIN(pipeline), encoder);
    if (muxer) gst_bin_add(GST_BIN(pipeline), muxer);
    if (payloader) gst_bin_add(GST_BIN(pipeline), payloader);
    if (sink) gst_bin_add(GST_BIN(pipeline), sink);

    // Step 12: Link Elements
    if (inputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK) {
        gst_element_link_many(source, depay, decoder, convert, NULL);
    } else if (inputData.esourceType == eSourceType::SOURCE_TYPE_FILE) {
        gst_element_link_many(source, demuxer, NULL);
        g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), this);
    }

    // if (parser) gst_element_link(decoder, parser);
    // if (encoder) gst_element_link(parser ? parser : decoder, encoder);
    // if (muxer) gst_element_link(encoder ? encoder : decoder, muxer);
    // if (payloader) gst_element_link(muxer ? muxer : encoder, payloader);
    // gst_element_link(payloader ? payloader : muxer ? muxer : encoder ? encoder : decoder, sink);

    gst_bin_add_many(GST_BIN(pipeline), source, depay, decoder, convert, sink, NULL);
    gst_element_link_many(depay, decoder, convert, sink, NULL);
    std::cerr << "Pipeline built successfully.\n";
    #endif
    /*MediaStreamDevice device = pipelineConfig.getMediaStreamDevice();
    MediaData inputData = device.stinputMediaData;
    MediaData outputData = device.stoutputMediaData;

    source = gst_element_factory_make("rtspsrc", "source");
    g_object_set(G_OBJECT(source), "location", device.sDeviceName.c_str(), "protocols", 0x00000004, NULL);
    g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added), this);

    if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264) {
        depay = gst_element_factory_make("rtph264depay", "depay");
        decoder = gst_element_factory_make("avdec_h264", "decoder");
    } else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265) {
        depay = gst_element_factory_make("rtph265depay", "depay");
        decoder = gst_element_factory_make("avdec_h265", "decoder");
    }

    convert = gst_element_factory_make("videoconvert", "convert");
    sink = gst_element_factory_make("autovideosink", "sink");

    gst_bin_add_many(GST_BIN(pipeline), source, depay, decoder, convert, sink, NULL);
    gst_element_link_many(depay, decoder, convert, sink, NULL);*/
}

void PipelineHandler::start() {
    std::cerr << "start pipeline \n";
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void PipelineHandler::pause() {
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
}

void PipelineHandler::stop() {
    gst_element_set_state(pipeline, GST_STATE_NULL);
}

void PipelineHandler::updatePipeline(const Pipeline& newPipeline) {
    stop();
    pipelineConfig = newPipeline;
    buildPipeline();
    start();
}
