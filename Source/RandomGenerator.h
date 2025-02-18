#include <iostream>
#include <array>
#include <cstdlib>
#include <ctime>
#include "Struct.h"
#include "Enum.h"
// Static class for random input generation
class RandomGenerator {
public:

    // Function to generate a random eVideoCodec
    static size_t getRandomValue() {
        size_t index = rand() % 100; // Avoid VIDEO_CODEC_NONE
        return index;
    }

    // Function to generate a random eVideoCodec
    static eVideoCodec getRandomVideoCodec() {
        int index = rand() % static_cast<int>(eVideoCodec::VIDEO_CODEC_MPEG4) + 1; // Avoid VIDEO_CODEC_NONE
        return static_cast<eVideoCodec>(index);
    }

    // Function to generate a random eAudioCodec
    static eAudioCodec getRandomAudioCodec() {
        int index = rand() % static_cast<int>(eAudioCodec::AUDIO_CODEC_WMA) + 1; // Avoid AUDIO_CODEC_NONE
        return static_cast<eAudioCodec>(index);
    }

    // Function to generate a random eAudioSampleRate
    static eAudioSampleRate getRandomAudioSampleRate() {
        int index = rand() % static_cast<int>(eAudioSampleRate::AUDIO_SAMPLE_RATE_K48) + 1; // Avoid AUDIO_SAMPLE_RATE_NONE
        return static_cast<eAudioSampleRate>(index);
    }

    // Function to generate a random eContainerFormat
    static eContainerFormat getRandomContainerFormat() {
        int index = rand() % static_cast<int>(eContainerFormat::CONTAINER_FORMAT_WebM) + 1; // Avoid CONTAINER_FORMAT_NONE
        return static_cast<eContainerFormat>(index);
    }

    // Function to generate a random eStreamingProtocol
    static eStreamingProtocol getRandomStreamingProtocol() {
        int index = rand() % static_cast<int>(eStreamingProtocol::STREAMING_PROTOCOL_RTMP) + 1; // Avoid STREAMING_PROTOCOL_NONE
        return static_cast<eStreamingProtocol>(index);
    }

    // Function to generate a random eSourceType
    static eSourceType getRandomSourceType() {
        int index = rand() % static_cast<int>(eSourceType::SOURCE_TYPE_NETWORK) + 1; // Avoid SOURCE_TYPE_NONE
        return static_cast<eSourceType>(index);
    }

    // Function to generate a random eAction
    static eAction getRandomAction() {
        int index = rand() % static_cast<int>(eAction::ACTION_PAUSE) + 1; // Avoid ACTION_NONE
        return static_cast<eAction>(index);
    }

    // Function to generate a random IP address
    static std::string getRandomIpAddress() {
        return std::to_string(rand() % 256) + "." + std::to_string(rand() % 256) + "." +
               std::to_string(rand() % 256) + "." + std::to_string(rand() % 256);
    }

    // Function to generate a random MediaStreamDevice structure
    static MediaStreamDevice generateRandomMediaStreamDevice(int deviceIndex) {
        MediaStreamDevice randomDevice;

        randomDevice.sDeviceName =  "camera-1" ;//"Device" + std::to_string(deviceIndex + 1);

        // randomDevice.stinputMediaData = MediaData(
        //     getRandomSourceType(),
        //     MediaCodec(getRandomVideoCodec(), getRandomAudioCodec(), getRandomAudioSampleRate()),
        //     MediaFileSource(getRandomContainerFormat()),
        //     NetworkStreaming(getRandomStreamingProtocol(), getRandomIpAddress(), rand() % 65536), // Random port number
        //     static_cast<eStreamingType>(rand() % static_cast<int>(eStreamingType::STREAMING_TYPE_VIDEO_SUBTITLE) + 1)
        // );
        randomDevice.stinputMediaData = MediaData(
            getRandomSourceType(),
            MediaCodec(getRandomVideoCodec(), getRandomAudioCodec(), getRandomAudioSampleRate()),
            MediaFileSource(getRandomContainerFormat()),
            NetworkStreaming(getRandomStreamingProtocol(), getRandomIpAddress(), rand() % 65536), // Random port number
            static_cast<eStreamingType>(rand() % static_cast<int>(eStreamingType::STREAMING_TYPE_VIDEO_SUBTITLE) + 1)
        );
        randomDevice.stoutputMediaData = MediaData(
            getRandomSourceType(),
            MediaCodec(getRandomVideoCodec(), getRandomAudioCodec(), getRandomAudioSampleRate()),
            MediaFileSource(getRandomContainerFormat()),
            NetworkStreaming(getRandomStreamingProtocol(), getRandomIpAddress(), rand() % 65536), // Random port number
            static_cast<eStreamingType>(rand() % static_cast<int>(eStreamingType::STREAMING_TYPE_VIDEO_SUBTITLE) + 1)
        );

        return randomDevice;
    }
};
