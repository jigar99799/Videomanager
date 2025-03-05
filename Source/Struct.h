#pragma once

#include "Enum.h"
#include <iostream>
#include <vector>
#include <string>

enum class MediaType {
	VIDEO,
	AUDIO,
	BOTH
};

enum class CodecType {
	H264,
	H265,
	VP8,
	VP9,
	AAC,
	MP3,
	OPUS
};

enum class PipelineOperation 
{
	Create,
	Update,
	Run,
	Start,
	Stop,
	Pause,
	Resume,
	Terminate
};


struct Resolution {
	int width{0};
	int height{0};
	float frameRate{0.0f};
};

struct NetworkConfig {
	std::string address;
	int port{0};
	std::string protocol;  // "udp", "rtp", "rtsp", etc.
	int bufferSize{0};
	int latency{0};
};

struct MediaCodec
{
	eVideoCodec evideocodec;
	eAudioCodec eaudiocodec;
	eAudioSampleRate eaudioSampleRate;
	
	// New members
	CodecType type;
	int bitrate ;
	std::string profile;
	std::string preset;
	std::string codecname;

	MediaCodec() : evideocodec(eVideoCodec::VIDEO_CODEC_NONE), eaudiocodec(eAudioCodec::AUDIO_CODEC_NONE), eaudioSampleRate(eAudioSampleRate::AUDIO_SAMPLE_RATE_NONE), type(CodecType::H264), bitrate(0), codecname("") {}

	MediaCodec(eVideoCodec video, eAudioCodec audio, eAudioSampleRate audioRate, std::string codec)
		: evideocodec(video), eaudiocodec(audio), eaudioSampleRate(audioRate), type(CodecType::H264), bitrate(0), codecname(codec) {}

	MediaCodec(const MediaCodec& other)
		: evideocodec(other.evideocodec), eaudiocodec(other.eaudiocodec), eaudioSampleRate(other.eaudioSampleRate), type(other.type), bitrate(other.bitrate), profile(other.profile), preset(other.preset), codecname(other.codecname) {}

	MediaCodec& operator=(const MediaCodec& other)
	{
		if (this != &other)
		{
			evideocodec = other.evideocodec;
			eaudiocodec = other.eaudiocodec;
			eaudioSampleRate = other.eaudioSampleRate;
			type = other.type;
			bitrate = other.bitrate;
			profile = other.profile;
			preset = other.preset;
			codecname = other.codecname;
		}
		return *this;
	}

	bool operator==(const MediaCodec& other) const
	{
		return (evideocodec == other.evideocodec &&
				eaudiocodec == other.eaudiocodec &&
				eaudioSampleRate == other.eaudioSampleRate &&
				type == other.type &&
				bitrate == other.bitrate &&
				profile == other.profile &&
				preset == other.preset &&
			codecname == other.codecname);
	}

	bool operator!=(const MediaCodec& other) const
	{
		return(!(*this == other));
	}

	~MediaCodec() {}
};

struct NetworkStreaming
{
	eStreamingProtocol estreamingProtocol;
	std::string sIpAddress;
	int iPort;

	NetworkStreaming()
		: estreamingProtocol(eStreamingProtocol::STREAMING_PROTOCOL_NONE), sIpAddress(""), iPort(0) {}

	NetworkStreaming(eStreamingProtocol protocol, const std::string& ipAddress, int port)
		: estreamingProtocol(protocol), sIpAddress(ipAddress), iPort(port) {}

	NetworkStreaming(const NetworkStreaming& other)
		: estreamingProtocol(other.estreamingProtocol), sIpAddress(other.sIpAddress), iPort(other.iPort) {}

	NetworkStreaming& operator=(const NetworkStreaming& other)
	{
		if (this != &other) {
			estreamingProtocol = other.estreamingProtocol;
			sIpAddress = other.sIpAddress;
			iPort = other.iPort;
		}
		return *this;
	}

	bool operator==(const NetworkStreaming& other) const
	{
		return (estreamingProtocol == other.estreamingProtocol &&
			sIpAddress == other.sIpAddress &&
			iPort == other.iPort);
	}

	bool operator!=(const NetworkStreaming& other) const
	{
		return(!(*this == other));
	}
};

struct MediaFileSource
{
	eContainerFormat econtainerFormat;

	MediaFileSource()
		: econtainerFormat(eContainerFormat::CONTAINER_FORMAT_NONE){}

	MediaFileSource(eContainerFormat format)
		: econtainerFormat(format){}

	MediaFileSource(const MediaFileSource& other)
		: econtainerFormat(other.econtainerFormat){}

	MediaFileSource& operator=(const MediaFileSource& other) {
		if (this != &other) { 
			econtainerFormat = other.econtainerFormat;
		}
		return *this;
	}

	bool operator==(const MediaFileSource& other) const
	{
		return (econtainerFormat == other.econtainerFormat);
	}

	bool operator!=(const MediaFileSource& other) const
	{
		return(!(*this == other));
	}
};

struct MediaData
{
	eSourceType esourceType;
	MediaCodec stMediaCodec;
	MediaFileSource stFileSource;
	NetworkStreaming stNetworkStreaming;
	eStreamingType estreamingType;

	MediaData()
		: esourceType(eSourceType::SOURCE_TYPE_NONE),estreamingType(eStreamingType::STREAMING_TYPE_NONE){}

	MediaData(eSourceType sourceType, MediaCodec mediaCodec, const MediaFileSource& fileSource,
		const NetworkStreaming& networkStreaming, eStreamingType streamingType)
		: esourceType(sourceType), stMediaCodec(mediaCodec), stFileSource(fileSource),
		stNetworkStreaming(networkStreaming), estreamingType(streamingType){}

	MediaData(const MediaData& other)
		: esourceType(other.esourceType), stMediaCodec(other.stMediaCodec),
		stFileSource(other.stFileSource), stNetworkStreaming(other.stNetworkStreaming),
		estreamingType(other.estreamingType){}

	MediaData& operator=(const MediaData& other) {
		if (this != &other) {
			esourceType = other.esourceType;
			stMediaCodec = other.stMediaCodec;
			stFileSource = other.stFileSource;
			stNetworkStreaming = other.stNetworkStreaming;
			estreamingType = other.estreamingType;
		}
		return *this;
	}

	bool operator==(const MediaData& other) const
	{
		return (esourceType == other.esourceType &&
			stMediaCodec == other.stMediaCodec &&
			stFileSource == other.stFileSource &&
			stNetworkStreaming == other.stNetworkStreaming &&
			estreamingType == other.estreamingType);
	}
	// Overload != operator (Inequality Check)
	bool operator!=(const MediaData& other) const
	{
		return !(*this == other);  // Negate the equality check
	}
};

struct MediaStreamDevice
{
	std::string sDeviceName;
	MediaData stinputMediaData;
	MediaData stoutputMediaData;
	std::string sourceOuputURL;

public:
	// Getters and setters
	const std::string& name() const { return sDeviceName; }
	void setName(const std::string& name) { sDeviceName = name; }
	
	const MediaData getinputmediadata() const { return stinputMediaData; }
	void setinputmediadata(MediaData inputdata) { stinputMediaData = inputdata; }

	const MediaData getoutputmediadata() const { return stoutputMediaData; }
	void setoutputmediadata(MediaData outdata) { stoutputMediaData = outdata; }


	// Default constructor
	MediaStreamDevice()
		: sDeviceName("")
		, stinputMediaData()
		, stoutputMediaData()
	    , sourceOuputURL() {}

	// Existing constructors
	MediaStreamDevice(const std::string& deviceName, const MediaData& inputMediaData, const MediaData& outputMediaData, std::string sourceOuputURL)
		: sDeviceName(deviceName)
		, stinputMediaData(inputMediaData)
		, stoutputMediaData(outputMediaData)
		, sourceOuputURL(sourceOuputURL) {}

	// Copy constructor
	MediaStreamDevice(const MediaStreamDevice& other)
		: sDeviceName(other.sDeviceName)
		, stinputMediaData(other.stinputMediaData)
		, stoutputMediaData(other.stoutputMediaData)
	    , sourceOuputURL(other.sourceOuputURL) {}

	// Assignment operator
	MediaStreamDevice& operator=(const MediaStreamDevice& other) {
		if (this != &other) {
			sDeviceName = other.sDeviceName;
			stinputMediaData = other.stinputMediaData;
			stoutputMediaData = other.stoutputMediaData;
			sourceOuputURL = other.sourceOuputURL;
		}
		return *this;
	}

	bool operator==(const MediaStreamDevice& other) const 
	{
		return (sDeviceName == other.sDeviceName &&
			stinputMediaData == other.stinputMediaData &&
			stoutputMediaData == other.stoutputMediaData && 
			sourceOuputURL == other.sourceOuputURL);
	}

	bool operator!=(const MediaStreamDevice& other) const 
	{
		return !(*this == other);
	}
};


// Structures to hold media stream information
struct AudioInfo {
	std::string codec;
	int channels;
	int sample_rate;
	int depth;
	int bitrate;
	int max_bitrate;
};

struct VideoInfo {
	std::string codec;
	int width;
	int height;
	int depth;
	float frame_rate;
	float pixel_aspect_ratio;
	bool is_interlaced;
	int bitrate;
	int max_bitrate;
};

struct SubtitleInfo {
	std::string codec;
	std::string language_name;
};

struct StreamInfo {
	std::vector<AudioInfo> audio_streams;
	std::vector<VideoInfo> video_streams;
	std::vector<SubtitleInfo> subtitle_streams;
};

