#if 0
// CMxPipeManager.cpp
#include "MxPipeManager.h"

void CMxPipeManager::addPipeline(const Pipeline& pipeline) {
    parsedQueue.enqueue(pipeline);
}

void CMxPipeManager::processQueue() {
    while (!parsedQueue.isEmpty()) {
        Pipeline currentPipeline = parsedQueue.dequeue();
        createPipeline(currentPipeline);
    }
}

// void CMxPipeManager::handlePipelineAction(const std::string& pipelineId, const std::string& requestId, const std::string& action) {
//     if (action == "create") {
//         processQueue(); // Process the queue to create pipelines
//     } else if (action == "play") {
//         startPipeline(pipelineId);
//     } else if (action == "pause") {
//         pausePipeline(pipelineId);
//     } else if (action == "stop") {
//         stopPipeline(pipelineId);
//     }
// }

void CMxPipeManager::createPipeline(const Pipeline& currentPipeline) 
{

        MediaStreamDevice inputDevice = currentPipeline.m_stMediaStreamDevice;
    
             // Create GStreamer pipeline
 gst_init(NULL,NULL);
        // Create GStreamer pipeline
        GstElement* pipeline = gst_pipeline_new("test-pipeline");
    
        // Create GStreamer elements based on input media data
        GstElement* source = nullptr;
    
        // Determine the source based on the MediaStreamDevice parameters
        if (inputDevice.stinputMediaData.esourceType == eSourceType::SOURCE_TYPE_NETWORK) {
            source = gst_element_factory_make("rtspsrc", "source");
            g_object_set(source, "location", inputDevice.stinputMediaData.stNetworkStreaming.sIpAddress.c_str(), nullptr);
        } else if (inputDevice.stinputMediaData.stMediaCodec.evideocodec == eVideoCodec::VIDEO_CODEC_FILE) {
            source = gst_element_factory_make("filesrc", "source");
            g_object_set(source, "location", inputDevice.stinputMediaData.stFileSource.econtainerFormat.c_str(), nullptr);
        }
    
        // Create specific decoder based on the codec type
        GstElement* decoder = nullptr;
        switch (inputDevice.stinputMediaData.stMediaCodec.evideocodec) {
            case eVideoCodec::VIDEO_CODEC_H264:
                decoder = gst_element_factory_make("avdec_h264", "h264decoder");
                break;
            case eVideoCodec::VIDEO_CODEC_H265:
                decoder = gst_element_factory_make("avdec_h265", "h265decoder");
                break;
            case eVideoCodec::VIDEO_CODEC_MP2V:
                decoder = gst_element_factory_make("avdec_mpeg2video", "mp2vdecoder");
                break;
            // Add more codec cases as needed
            default:
                g_printerr("Unsupported video codec for pipeline ID: %s\n", currentPipeline.pipelineId.c_str());
                return; // Skip this pipeline on failure
        }
    
        // Determine the sink based on the streaming type
        GstElement* sink = nullptr;
        if (inputDevice.stinputMediaData.estreamingType == eStreamingType::STREAMING_TYPE_LIVE) {
            sink = gst_element_factory_make("autovideosink", "sink"); // For video output
        } else if (inputDevice.stinputMediaData.estreamingType == eStreamingType::STREAMING_TYPE_FILE) {
            sink = gst_element_factory_make("filesink", "sink");
            g_object_set(sink, "location", "output.mp4", nullptr); // Example output file
        }
    
        // Check if elements were created successfully
        if (!source || !decoder || !sink) {
            g_printerr("Not all elements could be created for pipeline ID: %s\n", currentPipeline.pipelineId.c_str());
            return; // Skip this pipeline on failure
        }
    
        // Add elements to the pipeline
        gst_bin_add_many(GST_BIN(pipeline), source, decoder, sink, nullptr);
        gst_element_link(source, decoder);
        g_signal_connect(decoder, "pad-added", G_CALLBACK(on_pad_added), sink);
    
        // Store the pipeline in the map
        pipelines[currentPipeline.pipelineId] = pipeline;
    
        // Start the pipeline
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void CMxPipeManager::startPipeline(const std::string& pipelineId) {
    auto it = pipelines.find(pipelineId);
    if (it != pipelines.end()) {
        gst_element_set_state(it->second, GST_STATE_PLAYING);
    }

 // Create GStreamer pipeline
 gst_init(NULL,NULL);
 GstElement* pipeline = gst_pipeline_new("new-piplein");
 if (!pipeline) {
   ///  g_printerr("Failed to create pipeline for ID: %s\n", 1);
     return;
 }

 // Create GStreamer elements
 GstElement* source = gst_element_factory_make("rtspsrc", "source");
 GstElement* depay = gst_element_factory_make("rtph264depay", "depay");
 GstElement* decoder = gst_element_factory_make("avdec_h264", "decoder");
 GstElement* convert = gst_element_factory_make("videoconvert", "convert");
 GstElement* sink = gst_element_factory_make("autovideosink", "sink");

 if (!source || !depay || !decoder || !convert || !sink) {
 //    g_printerr("Failed to create one or more elements for pipeline ID: %s\n", 1);
     if (pipeline) gst_object_unref(pipeline);
     return;
 }

 // Set RTSP source properties
 g_object_set(source, "location", "rtsp://admin:admin@192.168.111.150/unicaststream/1", "protocols", 4, nullptr);

 // Add elements to the pipeline
 gst_bin_add_many(GST_BIN(pipeline), source, depay, decoder, convert, sink, nullptr);

 // Link static elements (dynamic link for decodebin is handled via signal)
 if (!gst_element_link(depay, decoder) ||
     !gst_element_link(decoder, convert) ||
     !gst_element_link(convert, sink)) {
    // g_printerr("Element linking failed for pipeline ID: %s\n",1);
     gst_object_unref(pipeline);
     return;
 }

 // Handle dynamic pad linking for rtspsrc
 g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added), depay);

 // Store the pipeline in the map
 //pipelines[currentPipeline.pipelineId] = 1;

 // Start the pipeline
 gst_element_set_state(pipeline, GST_STATE_PLAYING);


}

void CMxPipeManager::stopPipeline(const std::string& pipelineId) {
    auto it = pipelines.find(pipelineId);
    if (it != pipelines.end()) {
        gst_element_set_state(it->second, GST_STATE_NULL);
        gst_object_unref(it->second);
        pipelines.erase(it);
    }
}

void CMxPipeManager::pausePipeline(const std::string& pipelineId) {
    auto it = pipelines.find(pipelineId);
    if (it != pipelines.end()) {
        gst_element_set_state(it->second, GST_STATE_PAUSED);
    }
}

void CMxPipeManager::on_pad_added(GstElement* src, GstPad* pad, gpointer data) {
    GstElement* sink = GST_ELEMENT(data);
    GstPad* sink_pad = gst_element_get_static_pad(sink, "sink");
    gst_pad_link(pad, sink_pad);
    gst_object_unref(sink_pad);
}
#endif 
#include "MxPipelineManager.h"
#include <iostream>

PipelineManager::PipelineManager() : m_running(true), m_workerThread(&PipelineManager::processQueue, this) {}

PipelineManager::~PipelineManager() {
    m_running = false;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    for (auto& entry : m_pipelineHandlers) {
        delete entry.second;
    }
    m_pipelineHandlers.clear();
}

void PipelineManager::processQueue() {
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::lock_guard<std::mutex> lock(m_mutex);

        while (!m_pipelineQueue.empty()) {
            Pipeline pipelineData = m_pipelineQueue.front();
            m_pipelineQueue.pop();
            createPipelineInternal(pipelineData);
        }
    }
}

void PipelineManager::createPipelineInternal(const Pipeline& pipeline) {
    size_t pipelineID = pipeline.getPipelineID();
    if (m_pipelineHandlers.find(pipelineID) != m_pipelineHandlers.end()) {
        std::cerr << "Pipeline already exists. Updating instead.\n";
        m_pipelineHandlers[pipelineID]->updatePipeline(pipeline);
        return;
    }
    std::cerr << "Pipeline new create" << pipelineID <<std::endl;
    PipelineHandler* handler = new PipelineHandler(pipeline);
    m_pipelineHandlers[pipelineID] = handler;
}

void PipelineManager::enqueuePipeline(const Pipeline& pipeline) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pipelineQueue.push(pipeline);
}

void PipelineManager::startPipeline(size_t pipelineID) {
    if (m_pipelineHandlers.find(pipelineID) != m_pipelineHandlers.end()) {
        m_pipelineHandlers[pipelineID]->start();
    }
    else
    {
        std::cerr << "Pipeline start end received" << pipelineID <<std::endl;
    }
}

void PipelineManager::pausePipeline(size_t pipelineID) {
    if (m_pipelineHandlers.find(pipelineID) != m_pipelineHandlers.end()) {
        m_pipelineHandlers[pipelineID]->pause();
    }
}

void PipelineManager::stopPipeline(size_t pipelineID) {
    if (m_pipelineHandlers.find(pipelineID) != m_pipelineHandlers.end()) {
        m_pipelineHandlers[pipelineID]->stop();
        delete m_pipelineHandlers[pipelineID];
        m_pipelineHandlers.erase(pipelineID);
    }
}
