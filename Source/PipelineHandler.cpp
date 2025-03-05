#include "PipelineHandler.h"
#include <iostream>
#include <sstream>

PipelineHandler::PipelineHandler(const MediaStreamDevice& streamDevice)   : config(streamDevice) 
{
    //gst_init(nullptr, nullptr);

    m_state = State::INITIAL;
    
    // Handle XDG_RUNTIME_DIR environment variable for X11 systems
    const char* xdg_runtime = getenv("XDG_RUNTIME_DIR");
    if (!xdg_runtime) {
        MX_LOG_WARN("PipelineHandler", "XDG_RUNTIME_DIR not set in environment. This may affect display functionality.");
        // On some systems, we can set a default value
        #ifdef _WIN32
        // Windows doesn't use XDG_RUNTIME_DIR
        #else
        // For Linux/Unix systems
        const char* home = getenv("HOME");
        if (home) {
            std::string xdg_path = std::string(home) + "/.xdg_runtime";
            MX_LOG_INFO("PipelineHandler", ("Setting XDG_RUNTIME_DIR to " + xdg_path).c_str());
            setenv("XDG_RUNTIME_DIR", xdg_path.c_str(), 1);
        }
        #endif
    }

    buildPipeline();
}

PipelineHandler::~PipelineHandler() 
{
    terminate();
}

//bool PipelineHandler::buildPipeline() 
//{
//    if (!configurePipeline()) 
//    {
//        return false;
//    }
//    return true;
//}

void PipelineHandler::buildPipeline()
{

    // Get input and output configurations from PipelineManager
    MediaStreamDevice device = config;
    MediaData inputData = device.stinputMediaData;
    MediaData outputData = device.stoutputMediaData;

    //validate URL is proper or not
    if (!StreamDiscoverer::DiscoverStream(device.sDeviceName.c_str()))
    {
        std::string errorMsg = "RTSP URL is not reachable or not valid: " + device.sDeviceName;
        std::cerr << errorMsg << std::endl;
        MX_LOG_ERROR("PipelineHandler", errorMsg.c_str());
        handleError(errorMsg);
        return;
    }

    StreamInfo data = StreamDiscoverer::getStreamInfo();

    std::cerr << " buildpipeline api call\n";
    pipeline = gst_pipeline_new("pipeline");
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
        g_object_set(G_OBJECT(source), "do-rtsp-keep-alive", true, NULL);
        g_object_set(G_OBJECT(source), "protocols", 4, NULL); // 4 = GST_RTSP_LOWER_TRANS_TCP
        g_object_set(G_OBJECT(source), "retry", 3, NULL);
        g_object_set(G_OBJECT(source), "timeout", 5000000, NULL); // 5 seconds in microseconds
        g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added), this);
        std::cerr << "successfully create rtspsrc element\n";
    }
    else
    {
        std::cerr << "Unsupported source type\n";
        return;
    }

    // Step 2: Add Depayloader for RTP/RTSP sources
    if (inputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
    {

        std::cerr << "call  create depay element\n" << inputData.stMediaCodec.codecname;
        depay = CMx_DepayloaderFactory::createDepayloader("pipeliene", inputData.stMediaCodec.codecname);
    }

    //for audio parser G711-alaw
    if (device.stinputMediaData.stMediaCodec.eaudiocodec != eAudioCodec::AUDIO_CODEC_NONE)
        audiodepay = CMx_DepayloaderFactory::createDepayloaderforAudio("pipeliene", device.stinputMediaData.stMediaCodec.eaudiocodec);
    // audiodepay = gst_element_factory_make("rtppcmadepay", "audio depay");


     // Step 3: Add Parser for Compressed Formats
    parser = CMx_ParseFactory::createParser("pipeliene", inputData.stMediaCodec.codecname);

    // Step 4: Add Muxer if Output is a Container Format
    if (outputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MP4)
    {
        muxer = gst_element_factory_make("mp4mux", "muxer");
        // g_object_set(G_OBJECT(muxer), "faststart", true, NULL);
        // g_object_set(G_OBJECT(muxer), "fragment-duration", 1000000, NULL);
        g_object_set(G_OBJECT(muxer), "faststart", TRUE, "fragment-duration", 1000, "streamable", TRUE, "reserved-max-duration", 3000000000, NULL);

    }
    else if (outputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MKV)
    {
        muxer = gst_element_factory_make("matroskamux", "muxer");
    }


    gst_bin_add_many(GST_BIN(pipeline), source, depay, parser, audiodepay, NULL);
    gst_element_link(depay, parser);
    prevElement = parser;

    //Configuration is modify than changes accordingly3
    //MediaConfigurationChanges();

    // Step 5: Identify Sink Element
    if (outputData.esourceType == eSourceType::SOURCE_TYPE_FILE)
    {
        sink = gst_element_factory_make("filesink", "sink");
        g_object_set(G_OBJECT(sink), "location", "output_new.mp4", NULL);
    }
    else if (outputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
    {

        sink = gst_element_factory_make("rtspclientsink", "sink");
        g_object_set(G_OBJECT(sink), "location", device.sourceOuputURL.c_str(), NULL); //TODO bharat here modify
        std::cerr << "successfully create network element\n";
    }
    else if (outputData.esourceType == eSourceType::SOURCE_TYPE_DISPLAY)
    {
        if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
        {
            std::cerr << "successfully create decode element\n";
            decoder = gst_element_factory_make("avdec_h264", "decode");
        }
        else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
        {
            decoder = gst_element_factory_make("avdec_h265", "decode");
        }
        
        sink = gst_element_factory_make("autovideosink", "display");
        if (!sink) {
            std::string errorMsg = "Failed to create autovideosink element";
            MX_LOG_ERROR("PipelineHandler", errorMsg.c_str());
            handleError(errorMsg);
            return;
        }
        
        // Set sync property to true for live sources
        if (inputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK) {
            g_object_set(G_OBJECT(sink), "sync", TRUE, NULL);
        }
        
        std::cerr << "successfully create autovideosink element\n";
    }
    // Step 8: Add Elements to Pipeline
    if (outputData.esourceType == eSourceType::SOURCE_TYPE_FILE)
    {
        gst_bin_add_many(GST_BIN(pipeline), source, depay, parser, muxer, sink, NULL);
        gst_element_link_many(depay, parser, muxer, sink, NULL);
    }
    else if (outputData.esourceType == eSourceType::SOURCE_TYPE_DISPLAY)
    {
        std::cerr << "successfully link display  element\n";
        gst_bin_add_many(GST_BIN(pipeline), source, depay, parser, decoder, sink, NULL);
        gst_element_link_many(depay, parser, decoder, sink, NULL);
    }
    else if (outputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
    {
        gst_bin_add(GST_BIN(pipeline), sink);
        gst_element_link(prevElement, sink);
        if (device.stinputMediaData.stMediaCodec.eaudiocodec != eAudioCodec::AUDIO_CODEC_NONE)
        {
            if (!gst_element_link(audiodepay, sink))
            {
                g_printerr("Audio elements could not be linked. Exiting.\n");

            }
        }
    }

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, (GstBusFunc)PipelineHandler::busCallback, this);
    gst_object_unref(bus);
}

//bool PipelineHandler::configurePipeline() 
//{
//    MediaStreamDevice device = config;
//
//    try
//    {
//        // Get input and output configurations from PipelineManager
//        MediaData inputData = device.getinputmediadata();
//        MediaData outputData = device.getoutputmediadata();
//
//        // Create a new pipeline
//        elements.pipeline = gst_pipeline_new(device.name().c_str());
//        if (!elements.pipeline) {
//            handleError("Failed to create pipeline element");
//            return false;
//        }
//
//        // Step 1: Create Source Element
//        if (inputData.esourceType == eSourceType::SOURCE_TYPE_FILE)
//        {
//            elements.source = gst_element_factory_make("filesrc", "source");
//            if (!elements.source) {
//                handleError("Failed to create filesrc element");
//                return false;
//            }
//            g_object_set(G_OBJECT(elements.source), "location", device.name().c_str(), NULL);
//        }
//        else if (inputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
//        {
//            // For RTSP sources
//            if (inputData.stNetworkStreaming.estreamingProtocol == eStreamingProtocol::STREAMING_PROTOCOL_RTSP) 
//            {
//                elements.source = gst_element_factory_make("rtspsrc", "source");
//                if (!elements.source) {
//                    handleError("Failed to create rtspsrc element");
//                    return false;
//                }
//                
//                std::string rtspUrl = "rtsp://" + inputData.stNetworkStreaming.sIpAddress + ":" + 
//                                     std::to_string(inputData.stNetworkStreaming.iPort);
//                g_object_set(G_OBJECT(elements.source), "location", rtspUrl.c_str(), NULL);
//                
//                // Connect pad-added signal for dynamic pad creation
//                g_signal_connect(elements.source, "pad-added", G_CALLBACK(padAddedHandlerStatic), this);
//            }
//            // For UDP sources
//            else if (inputData.stNetworkStreaming.estreamingProtocol == eStreamingProtocol::STREAMING_PROTOCOL_RTP) 
//            {
//                elements.source = gst_element_factory_make("udpsrc", "source");
//                if (!elements.source) {
//                    handleError("Failed to create udpsrc element");
//                    return false;
//                }
//                
//                g_object_set(G_OBJECT(elements.source), "address", inputData.stNetworkStreaming.sIpAddress.c_str(), NULL);
//                g_object_set(G_OBJECT(elements.source), "port", inputData.stNetworkStreaming.iPort, NULL);
//            }
//            else 
//            {
//                handleError("Unsupported streaming protocol");
//                return false;
//            }
//        }
//        else
//        {
//            handleError("Unsupported source type");
//            return false;
//        }
//
//        // Step 2: Create Depayloader for RTP/RTSP sources
//        if (inputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK &&
//            inputData.stNetworkStreaming.estreamingProtocol == eStreamingProtocol::STREAMING_PROTOCOL_RTP)
//        {
//            if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
//            {
//                elements.depay = gst_element_factory_make("rtph264depay", "depay");
//                if (!elements.depay) {
//                    handleError("Failed to create rtph264depay element");
//                    return false;
//                }
//            }
//            else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
//            {
//                elements.depay = gst_element_factory_make("rtph265depay", "depay");
//                if (!elements.depay) {
//                    handleError("Failed to create rtph265depay element");
//                    return false;
//                }
//            }
//        }
//
//        // Step 3: Create Demuxer if Input is a Container Format
//        if (inputData.esourceType == eSourceType::SOURCE_TYPE_FILE)
//        {
//            if (inputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MP4)
//            {
//                elements.demuxer = gst_element_factory_make("qtdemux", "demuxer");
//                if (!elements.demuxer) {
//                    handleError("Failed to create qtdemux element");
//                    return false;
//                }
//                // Connect pad-added signal for dynamic pad creation from demuxer
//                g_signal_connect(elements.demuxer, "pad-added", G_CALLBACK(padAddedHandlerStatic), this);
//            }
//            else if (inputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MKV)
//            {
//                elements.demuxer = gst_element_factory_make("matroskademux", "demuxer");
//                if (!elements.demuxer) {
//                    handleError("Failed to create matroskademux element");
//                    return false;
//                }
//                // Connect pad-added signal for dynamic pad creation from demuxer
//                g_signal_connect(elements.demuxer, "pad-added", G_CALLBACK(padAddedHandlerStatic), this);
//            }
//        }
//
//        // Step 4: Create Parser for Compressed Formats
//        if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
//        {
//            elements.parser = gst_element_factory_make("h264parse", "parser");
//            if (!elements.parser) {
//                handleError("Failed to create h264parse element");
//                return false;
//            }
//        }
//        else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
//        {
//            elements.parser = gst_element_factory_make("h265parse", "parser");
//            if (!elements.parser) {
//                handleError("Failed to create h265parse element");
//                return false;
//            }
//        }
//
//        // Step 5: Create Decoder
//        if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
//        {
//            elements.decoder = gst_element_factory_make("avdec_h264", "decoder");
//            if (!elements.decoder) {
//                handleError("Failed to create avdec_h264 element");
//                return false;
//            }
//        }
//        else if (inputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
//        {
//            elements.decoder = gst_element_factory_make("avdec_h265", "decoder");
//            if (!elements.decoder) {
//                handleError("Failed to create avdec_h265 element");
//                return false;
//            }
//        }
//
//        // Step 6: Create Video Converter
//        elements.convert = gst_element_factory_make("videoconvert", "convert");
//        if (!elements.convert) {
//            handleError("Failed to create videoconvert element");
//            return false;
//        }
//
//        // Step 7: Create Encoder (If Required for output)
//        if (outputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
//        {
//            elements.encoder = gst_element_factory_make("x264enc", "encoder");
//            if (!elements.encoder) {
//                handleError("Failed to create x264enc element");
//                return false;
//            }
//            // Configure encoder properties for better performance
//            g_object_set(G_OBJECT(elements.encoder), "tune", 4, NULL); // zerolatency
//            g_object_set(G_OBJECT(elements.encoder), "speed-preset", 1, NULL); // ultrafast
//        }
//        else if (outputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
//        {
//            elements.encoder = gst_element_factory_make("x265enc", "encoder");
//            if (!elements.encoder) {
//                handleError("Failed to create x265enc element");
//                return false;
//            }
//            // Configure encoder properties
//            g_object_set(G_OBJECT(elements.encoder), "tune", 4, NULL); // zerolatency
//        }
//
//        // Step 8: Create Muxer if Output is a Container Format
//        if (outputData.esourceType == eSourceType::SOURCE_TYPE_FILE)
//        {
//            if (outputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MP4)
//            {
//                elements.muxer = gst_element_factory_make("mp4mux", "muxer");
//                if (!elements.muxer) {
//                    handleError("Failed to create mp4mux element");
//                    return false;
//                }
//            }
//            else if (outputData.stFileSource.econtainerFormat == eContainerFormat::CONTAINER_FORMAT_MKV)
//            {
//                elements.muxer = gst_element_factory_make("matroskamux", "muxer");
//                if (!elements.muxer) {
//                    handleError("Failed to create matroskamux element");
//                    return false;
//                }
//            }
//        }
//
//        // Step 9: Create Payloader for Network Streaming
//        if (outputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
//        {
//            if (outputData.stNetworkStreaming.estreamingProtocol == eStreamingProtocol::STREAMING_PROTOCOL_RTP)
//            {
//                if (outputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H264)
//                {
//                    elements.payloader = gst_element_factory_make("rtph264pay", "payloader");
//                    if (!elements.payloader) {
//                        handleError("Failed to create rtph264pay element");
//                        return false;
//                    }
//                }
//                else if (outputData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_H265)
//                {
//                    elements.payloader = gst_element_factory_make("rtph265pay", "payloader");
//                    if (!elements.payloader) {
//                        handleError("Failed to create rtph265pay element");
//                        return false;
//                    }
//                }
//            }
//        }
//
//        // Step 10: Create Sink Element
//        if (outputData.esourceType == eSourceType::SOURCE_TYPE_FILE)
//        {
//            elements.sink = gst_element_factory_make("filesink", "sink");
//            if (!elements.sink) {
//                handleError("Failed to create filesink element");
//                return false;
//            }
//            //g_object_set(G_OBJECT(elements.sink), "location", outputData.stFileSource.filePath.c_str(), NULL);
//        }
//        else if (outputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
//        {
//            elements.sink = gst_element_factory_make("udpsink", "sink");
//            if (!elements.sink) {
//                handleError("Failed to create udpsink element");
//                return false;
//            }
//            g_object_set(G_OBJECT(elements.sink), "host", outputData.stNetworkStreaming.sIpAddress.c_str(), NULL);
//            g_object_set(G_OBJECT(elements.sink), "port", outputData.stNetworkStreaming.iPort, NULL);
//        }
//        else
//        {
//            elements.sink = gst_element_factory_make("autovideosink", "sink");
//            if (!elements.sink) {
//                handleError("Failed to create autovideosink element");
//                return false;
//            }
//        }
//
//        // Step 11: Add Elements to the Pipeline - only add elements that were created
//        // Add source to pipeline
//        gst_bin_add(GST_BIN(elements.pipeline), elements.source);
//        
//        // Add other elements based on pipeline configuration
//        if (inputData.esourceType == eSourceType::SOURCE_TYPE_FILE)
//        {
//            // File source pipeline
//            if (elements.demuxer) {
//                gst_bin_add(GST_BIN(elements.pipeline), elements.demuxer);
//                // Link source to demuxer
//                if (!gst_element_link(elements.source, elements.demuxer)) {
//                    handleError("Failed to link source to demuxer");
//                    return false;
//                }
//                // Demuxer pads will be linked dynamically via pad-added signal
//            }
//        }
//        else if (inputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
//        {
//            if (inputData.stNetworkStreaming.estreamingProtocol == eStreamingProtocol::STREAMING_PROTOCOL_RTP)
//            {
//                // RTP pipeline
//                if (elements.depay) gst_bin_add(GST_BIN(elements.pipeline), elements.depay);
//                if (elements.parser) gst_bin_add(GST_BIN(elements.pipeline), elements.parser);
//                if (elements.decoder) gst_bin_add(GST_BIN(elements.pipeline), elements.decoder);
//                if (elements.convert) gst_bin_add(GST_BIN(elements.pipeline), elements.convert);
//                
//                // Link elements for RTP pipeline
//                if (elements.depay && elements.parser) {
//                    if (!gst_element_link(elements.source, elements.depay) ||
//                        !gst_element_link(elements.depay, elements.parser)) {
//                        handleError("Failed to link RTP source elements");
//                        return false;
//                    }
//                }
//                
//                if (elements.parser && elements.decoder) {
//                    if (!gst_element_link(elements.parser, elements.decoder)) {
//                        handleError("Failed to link parser to decoder");
//                        return false;
//                    }
//                }
//                
//                if (elements.decoder && elements.convert) {
//                    if (!gst_element_link(elements.decoder, elements.convert)) {
//                        handleError("Failed to link decoder to converter");
//                        return false;
//                    }
//                }
//            }
//            else if (inputData.stNetworkStreaming.estreamingProtocol == eStreamingProtocol::STREAMING_PROTOCOL_RTSP)
//            {
//                // RTSP pipeline - pads will be linked dynamically via pad-added signal
//                if (elements.depay) gst_bin_add(GST_BIN(elements.pipeline), elements.depay);
//                if (elements.parser) gst_bin_add(GST_BIN(elements.pipeline), elements.parser);
//                if (elements.decoder) gst_bin_add(GST_BIN(elements.pipeline), elements.decoder);
//                if (elements.convert) gst_bin_add(GST_BIN(elements.pipeline), elements.convert);
//                
//                // For RTSP, we don't link the source as it will be done dynamically
//                // Link the rest of the pipeline
//                if (elements.parser && elements.decoder) {
//                    if (!gst_element_link(elements.parser, elements.decoder)) {
//                        handleError("Failed to link parser to decoder");
//                        return false;
//                    }
//                }
//                
//                if (elements.decoder && elements.convert) {
//                    if (!gst_element_link(elements.decoder, elements.convert)) {
//                        handleError("Failed to link decoder to converter");
//                        return false;
//                    }
//                }
//            }
//        }
//        
//        // Add and link output elements
//        if (outputData.esourceType == eSourceType::SOURCE_TYPE_FILE)
//        {
//            // File output pipeline
//            if (elements.encoder) gst_bin_add(GST_BIN(elements.pipeline), elements.encoder);
//            if (elements.muxer) gst_bin_add(GST_BIN(elements.pipeline), elements.muxer);
//            if (elements.sink) gst_bin_add(GST_BIN(elements.pipeline), elements.sink);
//            
//            // Link converter to encoder if present
//            if (elements.convert && elements.encoder) {
//                if (!gst_element_link(elements.convert, elements.encoder)) {
//                    handleError("Failed to link converter to encoder");
//                    return false;
//                }
//            }
//            
//            // Link encoder to muxer if present
//            if (elements.encoder && elements.muxer) {
//                if (!gst_element_link(elements.encoder, elements.muxer)) {
//                    handleError("Failed to link encoder to muxer");
//                    return false;
//                }
//            }
//            
//            // Link muxer to sink if present
//            if (elements.muxer && elements.sink) {
//                if (!gst_element_link(elements.muxer, elements.sink)) {
//                    handleError("Failed to link muxer to sink");
//                    return false;
//                }
//            }
//            // If no muxer, link encoder directly to sink
//            else if (elements.encoder && elements.sink && !elements.muxer) {
//                if (!gst_element_link(elements.encoder, elements.sink)) {
//                    handleError("Failed to link encoder to sink");
//                    return false;
//                }
//            }
//            // If no encoder or muxer, link converter directly to sink
//            else if (elements.convert && elements.sink && !elements.encoder) {
//                if (!gst_element_link(elements.convert, elements.sink)) {
//                    handleError("Failed to link converter to sink");
//                    return false;
//                }
//            }
//        }
//        else if (outputData.esourceType == eSourceType::SOURCE_TYPE_NETWORK)
//        {
//            // Network output pipeline
//            if (elements.encoder) gst_bin_add(GST_BIN(elements.pipeline), elements.encoder);
//            if (elements.payloader) gst_bin_add(GST_BIN(elements.pipeline), elements.payloader);
//            if (elements.sink) gst_bin_add(GST_BIN(elements.pipeline), elements.sink);
//            
//            // Link converter to encoder if present
//            if (elements.convert && elements.encoder) {
//                if (!gst_element_link(elements.convert, elements.encoder)) {
//                    handleError("Failed to link converter to encoder");
//                    return false;
//                }
//            }
//            
//            // Link encoder to payloader if present
//            if (elements.encoder && elements.payloader) {
//                if (!gst_element_link(elements.encoder, elements.payloader)) {
//                    handleError("Failed to link encoder to payloader");
//                    return false;
//                }
//            }
//            
//            // Link payloader to sink if present
//            if (elements.payloader && elements.sink) {
//                if (!gst_element_link(elements.payloader, elements.sink)) {
//                    handleError("Failed to link payloader to sink");
//                    return false;
//                }
//            }
//            // If no payloader, link encoder directly to sink
//            else if (elements.encoder && elements.sink && !elements.payloader) {
//                if (!gst_element_link(elements.encoder, elements.sink)) {
//                    handleError("Failed to link encoder to sink");
//                    return false;
//                }
//            }
//            // If no encoder or payloader, link converter directly to sink
//            else if (elements.convert && elements.sink && !elements.encoder) {
//                if (!gst_element_link(elements.convert, elements.sink)) {
//                    handleError("Failed to link converter to sink");
//                    return false;
//                }
//            }
//        }
//        else
//        {
//            // Display output (autovideosink)
//            if (elements.sink) gst_bin_add(GST_BIN(elements.pipeline), elements.sink);
//            
//            // Link converter to sink
//            if (elements.convert && elements.sink) {
//                if (!gst_element_link(elements.convert, elements.sink)) {
//                    handleError("Failed to link converter to sink");
//                    return false;
//                }
//            }
//        }
//
//        // Set up bus watch for messages
//        GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(elements.pipeline));
//        gst_bus_add_watch(bus, busCallback, this);
//        gst_object_unref(bus);
//
//        MX_LOG_INFO("PipelineHandler", ("Pipeline successfully built for device: " + device.name()).c_str());
//        return true;
//    }
//    catch (const std::exception& e)
//    {
//        MX_LOG_ERROR("PipelineHandler", ("Failed to build pipeline for device: " + device.name() + " - " + e.what()).c_str());
//        cleanupPipeline();
//        return false;
//    }
//}

void PipelineHandler::cleanupPipeline() 
{
    // First set the pipeline to NULL state if it exists
    if (pipeline) 
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        
        // Wait for the state change to complete
        GstStateChangeReturn ret = gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            MX_LOG_ERROR("PipelineHandler", "Failed to set pipeline to NULL state during cleanup");
        }
    }

    // Unref all elements - the pipeline will unref its children, so we only need to unref the pipeline
    if (pipeline) {
        gst_object_unref(GST_OBJECT(pipeline));
    }
    
    // Update state
    m_state = State::INITIAL;
    
    MX_LOG_INFO("PipelineHandler", "Pipeline resources cleaned up");
}

bool PipelineHandler::start() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_state == State::PLAYING)
    {
        MX_LOG_INFO("PipelineHandler", "Pipeline is already in PLAYING state");
        return true;
    }
    
    MX_LOG_TRACE("PipelineHandler", "pipeline state set to start");
    
    // Set pipeline to PLAYING state
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    if (ret == GST_STATE_CHANGE_FAILURE) 
    {
        handleError("Failed to start pipeline - state change failed");
        return false;
    }
    else if (ret == GST_STATE_CHANGE_ASYNC)
    {
        MX_LOG_INFO("PipelineHandler", "Pipeline state change will happen asynchronously");
        
        // Wait for up to 5 seconds for the state change to complete
        GstState state;
        ret = gst_element_get_state(pipeline, &state, nullptr, 5 * GST_SECOND);
        
        if (ret == GST_STATE_CHANGE_FAILURE)
        {
            handleError("Failed to start pipeline - async state change failed");
            return false;
        }
        else if (ret == GST_STATE_CHANGE_ASYNC)
        {
            MX_LOG_INFO("PipelineHandler", "Pipeline is still changing state asynchronously");
        }
        else
        {
            MX_LOG_INFO("PipelineHandler", "Pipeline state change completed successfully");
        }
    }
    else
    {
        MX_LOG_INFO("PipelineHandler", "Pipeline state change completed immediately");
    }

    m_isRunning = true;
    
    // Start a thread to monitor the pipeline bus
    std::thread([this]() 
    {
        GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
        while (m_isRunning) 
        {
            GstMessage* msg = gst_bus_timed_pop_filtered(
                bus, 100 * GST_MSECOND,  // Check every 100ms instead of blocking indefinitely
                (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED | 
                                GST_MESSAGE_QOS | GST_MESSAGE_WARNING));

            if (msg) 
            {
                busCallback(bus, msg, this);
                gst_message_unref(msg);
            }
        }
        gst_object_unref(bus);
    }).detach(); // Runs in a separate thread


    m_state = State::PLAYING;

    MX_LOG_TRACE("PipelineHandler", "pipeline state set to start");
    reportStatus(PipelineStatus::Success, "Pipeline started successfully");

    return true;
}

bool PipelineHandler::pause() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_state != State::PLAYING)
    {
        return false;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) 
    {
        handleError("Failed to pause pipeline state is not changed");
        return false;
    }

    m_state = State::PAUSED;

    MX_LOG_TRACE("PipelineHandler", "pipeline state set to paused");

    reportStatus(PipelineStatus::Success, "Pipeline paused successfully");

    return true;
}

bool PipelineHandler::resume() 
{
    MX_LOG_TRACE("PipelineHandler", "pipeline resume");
    return start();  // Reuse start logic
}

bool PipelineHandler::stop() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_state == State::STOPPED)
    {
        return true;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_NULL);

    if (ret == GST_STATE_CHANGE_FAILURE) 
    {
        handleError("Failed to stop pipeline state is not changed");
        return false;
    }
    
    m_state = State::STOPPED;

    MX_LOG_TRACE("PipelineHandler", "pipeline state set to stopped");

    reportStatus(PipelineStatus::Success, "Pipeline stopped successfully");

    return true;
}

void PipelineHandler::terminate() 
{
    stop();
    cleanupPipeline();
}

bool PipelineHandler::updateConfiguration(const MediaStreamDevice& newConfig) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Store current state
    State previousState = m_state;
    
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

    buildPipeline();
    
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
    return m_state == State::PLAYING;
}

PipelineHandler::State PipelineHandler::getState() const 
{
    return m_state;
}

void PipelineHandler::handleError(const std::string& error) 
{
    m_state = State::ERROR;
    
    MX_LOG_ERROR("PipelineHandler", ("Pipeline error:" + error).c_str());
    
    reportStatus(PipelineStatus::Error, error);
}

void PipelineHandler::on_pad_added(GstElement* element, GstPad* pad, gpointer data)
{
    PipelineHandler* self = static_cast<PipelineHandler*>(data);
    self->linkDynamicPad(pad);
}

bool PipelineHandler::linkDynamicPad(GstPad* newPad) 
{
    GstCaps* caps = gst_pad_get_current_caps(newPad);
    if (!caps)
    {
        caps = gst_pad_query_caps(newPad, NULL);
    }

    const GstStructure* str = gst_caps_get_structure(caps, 0);
    const gchar* name = gst_structure_get_name(str);
    g_print("Pad added with caps: %s\n", name);

    if (g_str_has_prefix(name, "application/x-rtp"))
    {
        const gchar* media = gst_structure_get_string(str, "media");
        if (g_strcmp0(media, "video") == 0)
        {
            GstPad* sinkPad = gst_element_get_static_pad(depay, "sink");
            if (!sinkPad || gst_pad_is_linked(sinkPad)) {
                gst_object_unref(sinkPad);
                return 0;
            }
            if (gst_pad_link(newPad, sinkPad) != GST_PAD_LINK_OK)
            {
                std::cerr << "Failed to link dynamic pad\n";
            }
            else
                std::cerr << "successfully to link dynamic pad\n";
            gst_object_unref(sinkPad);
        }
       /* else if (g_strcmp0(media, "audio") == 0)
        {
            GstPad* sinkpad = gst_element_get_static_pad(audiodepay, "sink");
            if (GST_PAD_LINK_SUCCESSFUL(gst_pad_link(newPad, sinkpad)))
            {
                g_print("Linked audio pad successfully\n");
            }
            else
            {
                g_print("Failed to link audio pad\n");
            }
            gst_object_unref(sinkpad);
            std::cerr << "audio frame receive\n";
        }*/
    }


    return true;
}

gboolean PipelineHandler::busCallback(GstBus* bus, GstMessage* msg, gpointer data)
{
    PipelineHandler* handler = static_cast<PipelineHandler*>(data);
    
    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_ERROR:
        {
            GError* err = nullptr;
            gchar* debug_info = nullptr;
            
            gst_message_parse_error(msg, &err, &debug_info);
            
            std::string errorMsg = "GStreamer error: " + std::string(err->message);
            if (debug_info) {
                errorMsg += "\nDebug info: " + std::string(debug_info);
            }
            
            MX_LOG_ERROR("PipelineHandler", errorMsg.c_str());
            
            handler->handleError(errorMsg);
            
            g_clear_error(&err);
            g_free(debug_info);
            break;
        }
        
        case GST_MESSAGE_WARNING:
        {
            GError* err = nullptr;
            gchar* debug_info = nullptr;
            
            gst_message_parse_warning(msg, &err, &debug_info);
            
            std::string warningMsg = "GStreamer warning: " + std::string(err->message);
            if (debug_info) {
                warningMsg += "\nDebug info: " + std::string(debug_info);
            }
            
            MX_LOG_INFO("PipelineHandler", warningMsg.c_str());
            
            g_clear_error(&err);
            g_free(debug_info);
            break;
        }
        
        case GST_MESSAGE_INFO:
        {
            GError* err = nullptr;
            gchar* debug_info = nullptr;
            
            gst_message_parse_info(msg, &err, &debug_info);
            
            std::string infoMsg = "GStreamer info: " + std::string(err->message);
            if (debug_info) {
                infoMsg += "\nDebug info: " + std::string(debug_info);
            }
            
            MX_LOG_INFO("PipelineHandler", infoMsg.c_str());
            
            g_clear_error(&err);
            g_free(debug_info);
            break;
        }
        
        case GST_MESSAGE_EOS:
            MX_LOG_INFO("PipelineHandler", "End of stream reached");
            handler->handleEndOfStream();
            break;
            
        case GST_MESSAGE_STATE_CHANGED:
        {
            // Only process state changes of the pipeline, not its children
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(handler->pipeline)) 
            {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                
                std::string stateChange = " ++ Pipeline state changed from " + 
                                         std::string(gst_element_state_get_name(old_state)) + " to " + 
                                         std::string(gst_element_state_get_name(new_state));
                
                if (pending_state != GST_STATE_VOID_PENDING) {
                    stateChange += " (pending: " + std::string(gst_element_state_get_name(pending_state)) + ")";
                }
                
                MX_LOG_INFO("PipelineHandler", stateChange.c_str());
                
                // Update our internal state tracking
                switch (new_state) 
                {
                    case GST_STATE_READY:
                        handler->m_state = State::READY;
                        break;
                    case GST_STATE_PAUSED:
                        handler->m_state = State::PAUSED;
                        break;
                    case GST_STATE_PLAYING:
                        handler->m_state = State::PLAYING;
                        handler->reportStatus(PipelineStatus::Success, "Pipeline is now playing");
                        MX_LOG_INFO("PipelineHandler", "Pipeline reached PLAYING state - video should be visible now");
                        break;
                    case GST_STATE_NULL:
                        handler->m_state = State::STOPPED;
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        
        case GST_MESSAGE_BUFFERING:
        {
            gint percent = 0;
            gst_message_parse_buffering(msg, &percent);
            
            std::string bufferingMsg = "Buffering: " + std::to_string(percent) + "%";
            MX_LOG_INFO("PipelineHandler", bufferingMsg.c_str());
            
            // If buffering is not at 100%, pause the pipeline
            if (percent < 100) {
                gst_element_set_state(handler->pipeline, GST_STATE_PAUSED);
            } else {
                // Buffering complete, resume playback if we were playing
                if (handler->m_state == State::PLAYING) {
                    gst_element_set_state(handler->pipeline, GST_STATE_PLAYING);
                }
            }
            break;
        }
        
        case GST_MESSAGE_CLOCK_LOST:
            // Get a new clock
            MX_LOG_INFO("PipelineHandler", "Clock lost, getting a new one");
            gst_element_set_state(handler->pipeline, GST_STATE_PAUSED);
            gst_element_set_state(handler->pipeline, GST_STATE_PLAYING);
            break;
            
        default:
            // Unhandled message
            break;
    }
    
    // Return TRUE to keep the message handler installed
    return TRUE;
}

//// Static pad added handler
//void PipelineHandler::padAddedHandlerStatic(GstElement* src, GstPad* new_pad, gpointer data) {
//    PipelineHandler* handler = static_cast<PipelineHandler*>(data);
//    handler->padAddedHandler(src, new_pad);
//}
//
//// Member pad added handler
//void PipelineHandler::padAddedHandler(GstElement* src, GstPad* new_pad) {
//    GstCaps* new_pad_caps = nullptr;
//    GstStructure* new_pad_struct = nullptr;
//    const gchar* new_pad_type = nullptr;
//    GstPad* sink_pad = nullptr;
//    GstElement* next_element = nullptr;
//    
//    // Check the new pad's type
//    new_pad_caps = gst_pad_get_current_caps(new_pad);
//    if (!new_pad_caps) {
//        new_pad_caps = gst_pad_query_caps(new_pad, nullptr);
//    }
//    
//    new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
//    new_pad_type = gst_structure_get_name(new_pad_struct);
//    
//    MX_LOG_INFO("PipelineHandler", ("New pad of type: " + std::string(new_pad_type)).c_str());
//    
//    // Check if this is a video pad
//    if (g_str_has_prefix(new_pad_type, "video/x-h264")) {
//        // We need to link to the h264parse element
//        if (elements.parser) {
//            next_element = elements.parser;
//        } else if (elements.decoder) {
//            next_element = elements.decoder;
//        }
//    } 
//    else if (g_str_has_prefix(new_pad_type, "video/x-h265")) {
//        // We need to link to the h265parse element
//        if (elements.parser) {
//            next_element = elements.parser;
//        } else if (elements.decoder) {
//            next_element = elements.decoder;
//        }
//    }
//    else if (g_str_has_prefix(new_pad_type, "video/")) {
//        // Generic video, try to link to decoder or converter
//        if (elements.decoder) {
//            next_element = elements.decoder;
//        } else if (elements.convert) {
//            next_element = elements.convert;
//        }
//    }
//    
//    // If we found an element to link to
//    if (next_element) {
//        // Get the sink pad from the next element
//        sink_pad = gst_element_get_static_pad(next_element, "sink");
//
//        if (!sink_pad || gst_pad_link(new_pad, sink_pad))
//        {
//            gst_object_unref(sink_pad);
//        }
//
//        if (gst_pad_link(new_pad, sink_pad) != GST_PAD_LINK_OK)
//        {
//            MX_LOG_ERROR("PipelineHandler", "Failed to link pads");
//        } else {
//            MX_LOG_INFO("PipelineHandler", "Pads linked successfully");
//        }
//    }
//    
//    // Clean up
//    if (sink_pad) {
//        gst_object_unref(sink_pad);
//    }
//    if (new_pad_caps) {
//        gst_caps_unref(new_pad_caps);
//    }
//}
