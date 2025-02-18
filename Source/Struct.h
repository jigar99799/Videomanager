#pragma once

#include "Enum.h"
#include <iostream>
#include <vector>
#include <string>

// Forward declarations
enum class SourceType {
	FILE,
	NETWORK,
	DEVICE
};

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
	int bitrate{0};
	std::string profile;
	std::string preset;

	MediaCodec() : evideocodec(eVideoCodec::VIDEO_CODEC_NONE), eaudiocodec(eAudioCodec::AUDIO_CODEC_NONE), eaudioSampleRate(eAudioSampleRate::AUDIO_SAMPLE_RATE_NONE), type(CodecType::H264), bitrate(0) {}

	MediaCodec(eVideoCodec video, eAudioCodec audio, eAudioSampleRate audioRate)
		: evideocodec(video), eaudiocodec(audio), eaudioSampleRate(audioRate), type(CodecType::H264), bitrate(0) {}

	MediaCodec(const MediaCodec& other)
		: evideocodec(other.evideocodec), eaudiocodec(other.eaudiocodec), eaudioSampleRate(other.eaudioSampleRate), type(other.type), bitrate(other.bitrate), profile(other.profile), preset(other.preset) {}

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
		}
		return *this;
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
};

struct MediaData {
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
};

struct MediaStreamDevice
{
	std::string sDeviceName;
	MediaData stinputMediaData;
	MediaData stoutputMediaData;

	// New members as private fields
private:
	std::string m_name;
	SourceType m_sourceType{SourceType::NETWORK};
	MediaType m_mediaType{MediaType::VIDEO};
	std::string m_address;
	int m_port{0};
	std::string m_protocol;
	MediaCodec m_inputCodec;
	MediaCodec m_outputCodec;
	Resolution m_resolution;
	NetworkConfig m_networkConfig;
	std::vector<std::string> m_additionalParams;
	bool m_enableHardwareAcceleration{false};
	bool m_autoStart{false};
	int m_reconnectAttempts{3};
	int m_reconnectDelay{5000};

public:
	// Getters and setters
	const std::string& name() const { return m_name; }
	void setName(const std::string& name) { m_name = name; }
	
	SourceType sourceType() const { return m_sourceType; }
	void setSourceType(SourceType type) { m_sourceType = type; }
	
	MediaType mediaType() const { return m_mediaType; }
	void setMediaType(MediaType type) { m_mediaType = type; }
	
	const std::string& address() const { return m_address; }
	void setAddress(const std::string& addr) { m_address = addr; }
	
	int port() const { return m_port; }
	void setPort(int port) { m_port = port; }
	
	const std::string& protocol() const { return m_protocol; }
	void setProtocol(const std::string& proto) { m_protocol = proto; }
	
	const MediaCodec& inputCodec() const { return m_inputCodec; }
	MediaCodec& inputCodec() { return m_inputCodec; }
	void setInputCodec(const MediaCodec& codec) { m_inputCodec = codec; }
	
	const MediaCodec& outputCodec() const { return m_outputCodec; }
	MediaCodec& outputCodec() { return m_outputCodec; }
	void setOutputCodec(const MediaCodec& codec) { m_outputCodec = codec; }
	
	const Resolution& resolution() const { return m_resolution; }
	Resolution& resolution() { return m_resolution; }
	void setResolution(const Resolution& res) { m_resolution = res; }

	// Default constructor
	MediaStreamDevice()
		: sDeviceName("")
		, stinputMediaData()
		, stoutputMediaData() {}

	// Existing constructors
	MediaStreamDevice(const std::string& deviceName, const MediaData& inputMediaData, const MediaData& outputMediaData)
		: sDeviceName(deviceName)
		, stinputMediaData(inputMediaData)
		, stoutputMediaData(outputMediaData) {}

	// Copy constructor
	MediaStreamDevice(const MediaStreamDevice& other)
		: sDeviceName(other.sDeviceName)
		, stinputMediaData(other.stinputMediaData)
		, stoutputMediaData(other.stoutputMediaData)
		, m_name(other.m_name)
		, m_sourceType(other.m_sourceType)
		, m_mediaType(other.m_mediaType)
		, m_address(other.m_address)
		, m_port(other.m_port)
		, m_protocol(other.m_protocol)
		, m_inputCodec(other.m_inputCodec)
		, m_outputCodec(other.m_outputCodec)
		, m_resolution(other.m_resolution)
		, m_networkConfig(other.m_networkConfig)
		, m_additionalParams(other.m_additionalParams)
		, m_enableHardwareAcceleration(other.m_enableHardwareAcceleration)
		, m_autoStart(other.m_autoStart)
		, m_reconnectAttempts(other.m_reconnectAttempts)
		, m_reconnectDelay(other.m_reconnectDelay) {}

	// Assignment operator
	MediaStreamDevice& operator=(const MediaStreamDevice& other) {
		if (this != &other) {
			sDeviceName = other.sDeviceName;
			stinputMediaData = other.stinputMediaData;
			stoutputMediaData = other.stoutputMediaData;
			m_name = other.m_name;
			m_sourceType = other.m_sourceType;
			m_mediaType = other.m_mediaType;
			m_address = other.m_address;
			m_port = other.m_port;
			m_protocol = other.m_protocol;
			m_inputCodec = other.m_inputCodec;
			m_outputCodec = other.m_outputCodec;
			m_resolution = other.m_resolution;
			m_networkConfig = other.m_networkConfig;
			m_additionalParams = other.m_additionalParams;
			m_enableHardwareAcceleration = other.m_enableHardwareAcceleration;
			m_autoStart = other.m_autoStart;
			m_reconnectAttempts = other.m_reconnectAttempts;
			m_reconnectDelay = other.m_reconnectDelay;
		}
		return *this;
	}

	// Additional constructor
	MediaStreamDevice(const std::string& deviceName, 
					 SourceType srcType, 
					 MediaType medType,
					 const std::string& addr,
					 int prt,
					 const std::string& proto)
		: sDeviceName(deviceName)
		, m_name(deviceName)
		, m_sourceType(srcType)
		, m_mediaType(medType)
		, m_address(addr)
		, m_port(prt)
		, m_protocol(proto) {}
};
