#pragma once
// StreamDiscoverer.h
#ifndef STREAMDISCOVERER_H
#define STREAMDISCOVERER_H

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include "struct.h"

class StreamDiscoverer {
public:
	static bool DiscoverStream(const gchar *uri);
	static const StreamInfo& getStreamInfo();
	static void OutputStreamInfo();
private:
	static StreamInfo stream_info;
	static bool ProcessStreams(GstDiscovererInfo *info);
	static void ProcessVideoStream(GstDiscovererStreamInfo *stream_info_data);
	static void ProcessAudioStream(GstDiscovererStreamInfo *stream_info_data);
	static void ProcessSubtitleStream(GstDiscovererStreamInfo *stream_info_data);
	static std::string extractAfterLastSlash(const std::string& input);
	static std::string getCodecFromStreamInfo(GstDiscovererStreamInfo *stream_info_data);
};

#endif // STREAMDISCOVERER_H