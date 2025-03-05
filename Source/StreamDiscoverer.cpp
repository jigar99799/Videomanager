#include "StreamDiscoverer.h"
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include "mx_logger.h"


StreamInfo StreamDiscoverer::stream_info;

std::string StreamDiscoverer::extractAfterLastSlash(const std::string& input) {
	try {
		if (input.empty()) {
			throw std::invalid_argument("Input string is empty");
		}
		size_t pos = input.find_last_of('/');
		return (pos != std::string::npos) ? input.substr(pos + 1) : input;
	}
	catch (const std::exception& e) 
	{
		MX_LOG_ERROR("StreamDiscoverer", ("Error in extractAfterLastSlash: " + std::string(e.what())).c_str());
		return input;
	}
}

std::string StreamDiscoverer::getCodecFromStreamInfo(GstDiscovererStreamInfo *stream_info_data) {
	try {
		if (!stream_info_data) {
			throw std::runtime_error("Null stream info data");
		}

		GstCaps* caps = gst_discoverer_stream_info_get_caps(stream_info_data);
		if (!caps) {
			throw std::runtime_error("Failed to get caps from stream info");
		}

		GstStructure *structure = gst_caps_get_structure(caps, 0);
		if (!structure) {
			gst_caps_unref(caps);
			throw std::runtime_error("Failed to get structure from caps");
		}

		const char* name = gst_structure_get_name(structure);
		if (!name) {
			gst_caps_unref(caps);
			throw std::runtime_error("Failed to get structure name");
		}

		std::string result = extractAfterLastSlash(std::string(name));
		gst_caps_unref(caps);
		return result;
	}
	catch (const std::exception& e) 
	{
		MX_LOG_ERROR("StreamDiscoverer", ("Error in getCodecFromStreamInfo: " + std::string(e.what())).c_str());
		return "unknown";
	}
}

bool StreamDiscoverer::ProcessStreams(GstDiscovererInfo *info) {
	try {
		if (!info) {
			throw std::runtime_error("Null discoverer info");
		}

		// Get result status
		// GstDiscovererResult result = gst_discoverer_info_get_result(info);
		// if (result != GST_DISCOVERER_OK) {
		// 	std::stringstream ss;
		// 	ss << "Discovery failed: ";
		// 	switch (result) {
		// 	case GST_DISCOVERER_URI_INVALID:
		// 		ss << "Invalid URI";
		// 		break;
		// 	case GST_DISCOVERER_ERROR:
		// 		ss << "Generic error";
		// 		break;
		// 	case GST_DISCOVERER_TIMEOUT:
		// 		ss << "Discovery timeout";
		// 		break;
		// 	case GST_DISCOVERER_BUSY:
		// 		ss << "Discoverer busy";
		// 		break;
		// 	case GST_DISCOVERER_MISSING_PLUGINS:
		// 		ss << "Missing plugins";
		// 		break;
		// 	default:
		// 		ss << "Unknown error";
		// 	}
		// 	throw std::runtime_error(ss.str());
		// }

		GList* stream_list = gst_discoverer_info_get_stream_list(info);
		if (!stream_list) {
			throw std::runtime_error("No streams found in the media!");
		}

		std::cout << "Processing streams..." << std::endl;

		// Clear existing stream info
		stream_info.video_streams.clear();
		stream_info.audio_streams.clear();
		stream_info.subtitle_streams.clear();

		bool found_any_stream = false;

		for (GList *item = stream_list; item != NULL; item = item->next) {
			if (!item->data) 
			{
				MX_LOG_WARN("StreamDiscoverer", "Warning: Null stream data encountered, skipping...");
				continue;
			}

			GstDiscovererStreamInfo *stream_info_data = GST_DISCOVERER_STREAM_INFO(item->data);
			if (!stream_info_data) 
			{
				MX_LOG_WARN("StreamDiscoverer", "Warning: Failed to cast stream info, skipping...");
				continue;
			}

			const gchar *stream_type = gst_discoverer_stream_info_get_stream_type_nick(stream_info_data);
			if (!stream_type) 
			{
				MX_LOG_WARN("StreamDiscoverer", "Warning: Unknown stream type, skipping...");
				continue;
			}

			std::cout << "Found stream of type: " << stream_type << std::endl;

			try {
				if (g_strcmp0(stream_type, "video") == 0) {
					ProcessVideoStream(stream_info_data);
					found_any_stream = true;
				}
				else if (g_strcmp0(stream_type, "audio") == 0) {
					ProcessAudioStream(stream_info_data);
					found_any_stream = true;
				}
				else if (g_strcmp0(stream_type, "subtitle") == 0) {
					ProcessSubtitleStream(stream_info_data);
					found_any_stream = true;
				}
				else {
					MX_LOG_WARN("StreamDiscoverer", ("Warning: Unhandled stream type: " + std::string(stream_type)).c_str());
				}
			}
			catch (const std::exception& e) 
			{
				MX_LOG_ERROR("StreamDiscoverer", ("Error processing " + std::string(stream_type) + " stream: " + std::string(e.what())).c_str());
			}
		}

		gst_discoverer_stream_info_list_free(stream_list);

		if (!found_any_stream) {
			throw std::runtime_error("No valid streams were found");
		}

		return true;

	}
	catch (const std::exception& e) {
		MX_LOG_ERROR("StreamDiscoverer", ("Error in ProcessStreams : " + std::string(e.what())).c_str());
		return false;
	}
}

void StreamDiscoverer::ProcessVideoStream(GstDiscovererStreamInfo *stream_info_data) {
	try {
		if (!stream_info_data) {
			throw std::runtime_error("Null video stream info data");
		}

		GstDiscovererVideoInfo *video_info = GST_DISCOVERER_VIDEO_INFO(stream_info_data);
		if (!video_info) {
			throw std::runtime_error("Failed to get video info from stream");
		}

		VideoInfo video_data;
		video_data.codec = getCodecFromStreamInfo(stream_info_data);

		// Get video dimensions
		video_data.width = gst_discoverer_video_info_get_width(video_info);
		video_data.height = gst_discoverer_video_info_get_height(video_info);
		if (video_data.width == 0 || video_data.height == 0) {
			MX_LOG_ERROR("StreamDiscoverer", "Warning: Invalid video dimensions detected");
		}

		video_data.depth = gst_discoverer_video_info_get_depth(video_info);

		// Get frame rate
		guint fps_num = gst_discoverer_video_info_get_framerate_num(video_info);
		guint fps_denom = gst_discoverer_video_info_get_framerate_denom(video_info);
		if (fps_denom == 0) {
			throw std::runtime_error("Invalid frame rate denominator (zero)");
		}
		video_data.frame_rate = static_cast<float>(fps_num) / fps_denom;

		// Get pixel aspect ratio
		guint par_num = gst_discoverer_video_info_get_par_num(video_info);
		guint par_denom = gst_discoverer_video_info_get_par_denom(video_info);
		if (par_denom == 0) {
			MX_LOG_WARN("StreamDiscoverer", "Warning: Invalid pixel aspect ratio denominator, defaulting to 1.0");
			video_data.pixel_aspect_ratio = 1.0;
		}
		else {
			video_data.pixel_aspect_ratio = static_cast<float>(par_num) / par_denom;
		}

		video_data.is_interlaced = gst_discoverer_video_info_is_interlaced(video_info);
		video_data.bitrate = gst_discoverer_video_info_get_bitrate(video_info);
		video_data.max_bitrate = gst_discoverer_video_info_get_max_bitrate(video_info);

		if (video_data.bitrate == 0) {
			MX_LOG_WARN("StreamDiscoverer", "Warning: Bitrate is 0, might indicate missing information");
		}

		stream_info.video_streams.push_back(video_data);

	}
	catch (const std::exception& e) {
		MX_LOG_ERROR("StreamDiscoverer", ("Error in ProcessVideoStream: " + std::string(e.what())).c_str());
		throw; // Re-throw to let caller handle it
	}
}

void StreamDiscoverer::ProcessAudioStream(GstDiscovererStreamInfo *stream_info_data) {
	try {
		if (!stream_info_data) {
			throw std::runtime_error("Null audio stream info data");
		}

		GstDiscovererAudioInfo *audio_info = GST_DISCOVERER_AUDIO_INFO(stream_info_data);
		if (!audio_info) {
			throw std::runtime_error("Failed to get audio info from stream");
		}

		AudioInfo audio_data;
		audio_data.codec = getCodecFromStreamInfo(stream_info_data);

		// Get audio channels
		audio_data.channels = gst_discoverer_audio_info_get_channels(audio_info);
		if (audio_data.channels == 0) {
			MX_LOG_WARN("StreamDiscoverer", "Warning: No audio channels detected");
		}

		// Get sample rate
		audio_data.sample_rate = gst_discoverer_audio_info_get_sample_rate(audio_info);
		if (audio_data.sample_rate == 0) {
			MX_LOG_WARN("StreamDiscoverer", "Warning: Invalid sample rate (0 Hz)");
		}

		audio_data.depth = gst_discoverer_audio_info_get_depth(audio_info);
		if (audio_data.depth == 0) {
			MX_LOG_WARN("StreamDiscoverer", "Warning: Audio depth is 0, might indicate missing information");
		}

		audio_data.bitrate = gst_discoverer_audio_info_get_bitrate(audio_info);
		audio_data.max_bitrate = gst_discoverer_audio_info_get_max_bitrate(audio_info);

		if (audio_data.bitrate == 0) {
			MX_LOG_WARN("StreamDiscoverer", "Warning: Audio bitrate is 0, might indicate missing information");
		}

		stream_info.audio_streams.push_back(audio_data);

	}
	catch (const std::exception& e) {
		MX_LOG_ERROR("StreamDiscoverer", ("Error in ProcessAudioStream: " + std::string(e.what())).c_str());;
		throw; // Re-throw to let caller handle it
	}
}

void StreamDiscoverer::ProcessSubtitleStream(GstDiscovererStreamInfo *stream_info_data) {
	try {
		if (!stream_info_data) {
			throw std::runtime_error("Null subtitle stream info data");
		}

		GstDiscovererSubtitleInfo *subtitle_info = GST_DISCOVERER_SUBTITLE_INFO(stream_info_data);
		if (!subtitle_info) {
			throw std::runtime_error("Failed to get subtitle info from stream");
		}

		SubtitleInfo subtitle_data;
		subtitle_data.codec = getCodecFromStreamInfo(stream_info_data);

		const gchar* lang = gst_discoverer_subtitle_info_get_language(subtitle_info);
		subtitle_data.language_name = lang ? lang : "unknown";

		stream_info.subtitle_streams.push_back(subtitle_data);

	}
	catch (const std::exception& e) {
		MX_LOG_ERROR("StreamDiscoverer", ("Error in ProcessSubtitleStream: " + std::string(e.what())).c_str());
		throw; // Re-throw to let caller handle it
	}
}

void StreamDiscoverer::OutputStreamInfo() {
	try {
		if (stream_info.video_streams.empty() &&
			stream_info.audio_streams.empty() &&
			stream_info.subtitle_streams.empty()) {
			std::cout << "No streams available to display." << std::endl;
			return;
		}

		if (!stream_info.video_streams.empty()) {
			std::cout << "Video Streams: " << std::endl;
			for (const auto &video : stream_info.video_streams) {
				std::cout << "  Codec: " << video.codec
					<< ", Resolution: " << video.width << "x" << video.height
					<< ", Frame Rate: " << video.frame_rate << " fps"
					<< ", Bitrate: " << (video.bitrate ? std::to_string(video.bitrate) : "unknown") << " bps"
					<< ", Interlaced: " << (video.is_interlaced ? "yes" : "no")
					<< ", Depth: " << video.depth << " bits"
					<< ", PAR: " << video.pixel_aspect_ratio << std::endl;
			}
		}

		if (!stream_info.audio_streams.empty()) {
			std::cout << "Audio Streams: " << std::endl;
			for (const auto &audio : stream_info.audio_streams) {
				std::cout << "  Codec: " << audio.codec
					<< ", Channels: " << audio.channels
					<< ", Sample Rate: " << audio.sample_rate << " Hz"
					<< ", Bitrate: " << (audio.bitrate ? std::to_string(audio.bitrate) : "unknown") << " bps"
					<< ", Depth: " << audio.depth << " bits" << std::endl;
			}
		}

		if (!stream_info.subtitle_streams.empty()) {
			std::cout << "Subtitle Streams: " << std::endl;
			for (const auto &subtitle : stream_info.subtitle_streams) {
				std::cout << "  Codec: " << subtitle.codec
					<< ", Language: " << subtitle.language_name << std::endl;
			}
		}

	}
	catch (const std::exception& e) {
		MX_LOG_ERROR("StreamDiscoverer", ("Error in OutputStreamInfo: " + std::string(e.what())).c_str());
	}
}

const StreamInfo& StreamDiscoverer::getStreamInfo() {
	return stream_info;
}

bool StreamDiscoverer::DiscoverStream(const gchar *uri) {
	// Clear previous stream info

    std::cerr << "temp print :  "  << uri <<std::endl;
	stream_info.video_streams.clear();
	stream_info.audio_streams.clear();
	stream_info.subtitle_streams.clear();

	GError *error = NULL;
	// Increase timeout to 15 seconds for slower networks
	GstDiscoverer *discoverer = gst_discoverer_new(15 * GST_SECOND, &error);

	if (error != NULL) {
		std::cerr << "Failed to create GstDiscoverer: " << error->message << std::endl;
		g_error_free(error);
		return false;
	}

	GstDiscovererInfo *info = gst_discoverer_discover_uri(discoverer, uri, &error);

	if (error != NULL) {
		std::cerr << "Failed to discover URI: " << uri << " Error: " << error->message << std::endl;
		MX_LOG_ERROR("StreamDiscoverer", ("Failed to discover URI: " + std::string(uri) + " Error: " + std::string(error->message)).c_str());
		g_error_free(error);
		g_object_unref(discoverer);
		return false;
	}
	std::cout << "Stream discovery successful." << std::endl;

	if (!ProcessStreams(info)) {
		std::cerr << "Failed to process streams." << std::endl;
		g_object_unref(discoverer);
		g_object_unref(info);
		return false;
	}

	g_object_unref(discoverer);
	g_object_unref(info);
	return true;
}