#pragma once

#include "Enum.h"
#include <iostream>

struct MediaCodec
{
	eVideoCodec evideocodec;
	eAudioCodec eaudiocodec;
	eAudioSampleRate eaudioSampleRate;

	MediaCodec() : evideocodec(eVideoCodec::VIDEO_CODEC_NONE), eaudiocodec(eAudioCodec::AUDIO_CODEC_NONE), eaudioSampleRate(eAudioSampleRate::AUDIO_SAMPLE_RATE_NONE) {}

	MediaCodec(eVideoCodec video, eAudioCodec audio, eAudioSampleRate audioRate)
		: evideocodec(video), eaudiocodec(audio), eaudioSampleRate(audioRate) {}

	MediaCodec(const MediaCodec& other)
		: evideocodec(other.evideocodec), eaudiocodec(other.eaudiocodec), eaudioSampleRate(other.eaudioSampleRate) {}

	MediaCodec& operator=(const MediaCodec& other)
	{
		if (this != &other)
		{
			evideocodec = other.evideocodec;
			eaudiocodec = other.eaudiocodec;
			eaudioSampleRate = other.eaudioSampleRate;
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

    // Default constructor
    MediaStreamDevice()
        : sDeviceName(""), stinputMediaData(), stoutputMediaData() {}

    // Parameterized constructor
    MediaStreamDevice(const std::string& deviceName, const MediaData& inputMediaData, const MediaData& outputMediaData)
        : sDeviceName(deviceName), stinputMediaData(inputMediaData), stoutputMediaData(outputMediaData) {}

    // Copy constructor
    MediaStreamDevice(const MediaStreamDevice& other)
        : sDeviceName(other.sDeviceName),
          stinputMediaData(other.stinputMediaData),
          stoutputMediaData(other.stoutputMediaData) {}

    // Assignment operator
    MediaStreamDevice& operator=(const MediaStreamDevice& other)
    {
        if (this != &other) {
            sDeviceName = other.sDeviceName;
            stinputMediaData = other.stinputMediaData;
            stoutputMediaData = other.stoutputMediaData;
        }
        return *this;
    }
};
