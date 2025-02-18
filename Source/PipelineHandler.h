#ifndef PIPELINE_HANDLER_H
#define PIPELINE_HANDLER_H

#include <gst/gst.h>
#include <mutex>
#include "Pipeline.h"

class PipelineHandler 
{
private:
    GstElement* pipeline;
    GstElement* source;
    GstElement* depay;
    GstElement* demuxer;
    GstElement* parser;
    GstElement* decoder;
    GstElement* muxer;
    GstElement* payloader;
    GstElement* encoder;
    GstElement* convert;
    GstElement* sink;
    std::mutex mtx;
    Pipeline pipelineConfig;

    static void on_pad_added(GstElement* src, GstPad* newPad, gpointer data);
    void linkDynamicPad(GstPad* newPad);
    void buildPipeline();

public:
    PipelineHandler(const Pipeline& pipelineData);
    ~PipelineHandler();

    void start();
    void pause();
    void stop();
    void updatePipeline(const Pipeline& newPipeline);
};

#endif  // PIPELINE_HANDLER_H
