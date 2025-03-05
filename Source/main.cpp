#include <iostream>
#include "PipelineRequest.h"
#include "PipelineProcess.h"

// Helper function to create a test MediaStreamDevice
PipelineRequest createTestDevice(const std::string& rtspUrl, eSourceType sourceType)
{
    PipelineRequest request;

    static int i = 0;
    i++;
    request.setPipelineID(i); 
    request.setRequestID(i); 
    request.setEAction(eAction::ACTION_CREATE);

    MediaStreamDevice device;
    device.sDeviceName = "rtsp://admin:admin@192.168.111.150/unicaststream/1";
    device.stinputMediaData.esourceType = eSourceType::SOURCE_TYPE_NETWORK;
    device.stinputMediaData.stNetworkStreaming.estreamingProtocol = eStreamingProtocol::STREAMING_PROTOCOL_RTSP;
    device.stinputMediaData.stMediaCodec.evideocodec = eVideoCodec::VIDEO_CODEC_H264;
    device.stinputMediaData.stMediaCodec.eaudiocodec = eAudioCodec::AUDIO_CODEC_G711_ALAW;
    device.stinputMediaData.stMediaCodec.codecname = "video/H264";
    device.stinputMediaData.stMediaCodec.type = CodecType::H264;
    device.stinputMediaData.stMediaCodec.bitrate = 20000;
    device.stinputMediaData.stNetworkStreaming.sIpAddress = "192.168.111.11";
    device.stinputMediaData.stNetworkStreaming.iPort = 1050;
    device.stoutputMediaData.esourceType = eSourceType::SOURCE_TYPE_DISPLAY;
    device.stoutputMediaData.stMediaCodec.type = CodecType::H264;
    request.setMediaStreamDevice(device);

    return request;
}

PipelineRequest StartTestDevice(const std::string& rtspUrl, eSourceType sourceType)
{
    PipelineRequest request;

    static int i = 0;
    i++;
    request.setPipelineID(i);
    request.setRequestID(i);
    request.setEAction(eAction::ACTION_START);

    MediaStreamDevice device;
    device.sDeviceName = "rtsp://admin:admin@192.168.111.150/unicaststream/1";
    device.stinputMediaData.esourceType = eSourceType::SOURCE_TYPE_NETWORK;
    device.stinputMediaData.stNetworkStreaming.estreamingProtocol = eStreamingProtocol::STREAMING_PROTOCOL_RTSP;
    device.stinputMediaData.stMediaCodec.evideocodec = eVideoCodec::VIDEO_CODEC_H264;
    device.stinputMediaData.stMediaCodec.eaudiocodec = eAudioCodec::AUDIO_CODEC_G711_ALAW;
    device.stinputMediaData.stMediaCodec.codecname = "video/H264";
    device.stinputMediaData.stMediaCodec.type = CodecType::H264;
    device.stinputMediaData.stMediaCodec.bitrate = 20000;
    device.stinputMediaData.stNetworkStreaming.sIpAddress = "192.168.111.11";
    device.stinputMediaData.stNetworkStreaming.iPort = 1050;
    device.stoutputMediaData.esourceType = eSourceType::SOURCE_TYPE_DISPLAY;
    device.stoutputMediaData.stMediaCodec.type = CodecType::H264;
    request.setMediaStreamDevice(device);

    return request;
}

PipelineRequest PauseTestDevice(const std::string& rtspUrl, eSourceType sourceType)
{
    PipelineRequest request;

    static int i = 0;
    i++;
    request.setPipelineID(i);
    request.setRequestID(i);
    request.setEAction(eAction::ACTION_PAUSE);

    MediaStreamDevice device;
    device.sDeviceName = "rtsp://admin:admin@192.168.111.150/unicaststream/1";
    device.stinputMediaData.esourceType = eSourceType::SOURCE_TYPE_NETWORK;
    device.stinputMediaData.stNetworkStreaming.estreamingProtocol = eStreamingProtocol::STREAMING_PROTOCOL_RTSP;
    device.stinputMediaData.stMediaCodec.evideocodec = eVideoCodec::VIDEO_CODEC_H264;
    device.stinputMediaData.stMediaCodec.type = CodecType::H264;
    device.stinputMediaData.stMediaCodec.bitrate = 20000;
    device.stinputMediaData.stNetworkStreaming.sIpAddress = "192.168.111.11";
    device.stinputMediaData.stNetworkStreaming.iPort = 1050;
    device.stoutputMediaData.esourceType = eSourceType::SOURCE_TYPE_DISPLAY;
    device.stoutputMediaData.stMediaCodec.type = CodecType::H264;
    request.setMediaStreamDevice(device);

    return request;
}

PipelineRequest ResumeTestDevice(const std::string& rtspUrl, eSourceType sourceType)
{
    PipelineRequest request;

    static int i = 0;
    i++;
    request.setPipelineID(i);
    request.setRequestID(i);
    request.setEAction(eAction::ACTION_RESUME);

    MediaStreamDevice device;
    device.sDeviceName = "rtsp://admin:admin@192.168.111.150/unicaststream/1";
    device.stinputMediaData.esourceType = eSourceType::SOURCE_TYPE_NETWORK;
    device.stinputMediaData.stNetworkStreaming.estreamingProtocol = eStreamingProtocol::STREAMING_PROTOCOL_RTSP;
    device.stinputMediaData.stMediaCodec.evideocodec = eVideoCodec::VIDEO_CODEC_H264;
    device.stinputMediaData.stMediaCodec.type = CodecType::H264;
    device.stinputMediaData.stMediaCodec.bitrate = 20000;
    device.stinputMediaData.stNetworkStreaming.sIpAddress = "192.168.111.11";
    device.stinputMediaData.stNetworkStreaming.iPort = 1050;
    device.stoutputMediaData.esourceType = eSourceType::SOURCE_TYPE_DISPLAY;
    device.stoutputMediaData.stMediaCodec.type = CodecType::H264;
    request.setMediaStreamDevice(device);

    return request;
}

PipelineRequest StopTestDevice(const std::string& rtspUrl, eSourceType sourceType)
{
    PipelineRequest request;

    static int i = 0;
    i++;
    request.setPipelineID(i);
    request.setRequestID(i);
    request.setEAction(eAction::ACTION_STOP);

    MediaStreamDevice device;
    device.sDeviceName = "rtsp://admin:admin@192.168.111.150/unicaststream/1";
    device.stinputMediaData.esourceType = eSourceType::SOURCE_TYPE_NETWORK;
    device.stinputMediaData.stNetworkStreaming.estreamingProtocol = eStreamingProtocol::STREAMING_PROTOCOL_RTSP;
    device.stinputMediaData.stMediaCodec.evideocodec = eVideoCodec::VIDEO_CODEC_H264;
    device.stinputMediaData.stMediaCodec.type = CodecType::H264;
    device.stinputMediaData.stMediaCodec.bitrate = 20000;
    device.stinputMediaData.stNetworkStreaming.sIpAddress = "192.168.111.11";
    device.stinputMediaData.stNetworkStreaming.iPort = 1050;
    device.stoutputMediaData.esourceType = eSourceType::SOURCE_TYPE_DISPLAY;
    device.stoutputMediaData.stMediaCodec.type = CodecType::H264;
    request.setMediaStreamDevice(device);

    return request;
}


// New unified callback function
void pipelineCallback(
    PipelineStatus status,
    size_t pipelineId,
    size_t requestId,
    const std::string& message)
{
    // Format the status as a string
    std::string statusStr;
    switch (status) {
        case PipelineStatus::Success:
            statusStr = "SUCCESS";
            break;
        case PipelineStatus::InProgress:
            statusStr = "IN_PROGRESS";
            break;
        case PipelineStatus::Error:
            statusStr = "ERROR";
            break;
        case PipelineStatus::NetworkError:
            statusStr = "NETWORK_ERROR";
            break;
        case PipelineStatus::ConfigError:
            statusStr = "CONFIG_ERROR";
            break;
        case PipelineStatus::ResourceError:
            statusStr = "RESOURCE_ERROR";
            break;
        case PipelineStatus::Timeout:
            statusStr = "TIMEOUT";
            break;
        case PipelineStatus::Cancelled:
            statusStr = "CANCELLED";
            break;
        default:
            statusStr = "UNKNOWN";
            break;
    }

    // Print the formatted message
    std::cout << "====================" << std::endl;
    std::cout << "Pipeline " << pipelineId << " (Request " << requestId << "): " << statusStr << std::endl;
    std::cout << "Message: " << message << std::endl;
    std::cout << "====================" << std::endl;
}

int main()
{
    try
    {
        // Initialize the pipeline process with our new callback
        if (!PipelineProcess::initialize("debug_config.json", pipelineCallback))
        {
            std::cerr << "Failed to initialize pipeline process" << std::endl;
            return 1;
        }
      
        // Main application loop
        while (true)
        {
            // Here you would typically:
            // 1. Listen for socket connections
            // 2. Handle UI events
            // 3. Process other system events

            PipelineRequest request  = createTestDevice("rtsp://test", eSourceType::SOURCE_TYPE_NETWORK);
            PipelineProcess::enqueueRequest(request);

            //std::this_thread::sleep_for(std::chrono::seconds(15));

            PipelineRequest  request1 = StartTestDevice("rtsp://test", eSourceType::SOURCE_TYPE_NETWORK);
            PipelineProcess::enqueueRequest(request1);

            std::this_thread::sleep_for(std::chrono::seconds(150));

            // request = PauseTestDevice("rtsp://test", eSourceType::SOURCE_TYPE_NETWORK);
            // PipelineProcess::enqueueRequest(request);

            // std::this_thread::sleep_for(std::chrono::seconds(15));

            // request = ResumeTestDevice("rtsp://test", eSourceType::SOURCE_TYPE_NETWORK);
            // PipelineProcess::enqueueRequest(request);

            // std::this_thread::sleep_for(std::chrono::seconds(15));

            // request = StopTestDevice("rtsp://test", eSourceType::SOURCE_TYPE_NETWORK);
            // PipelineProcess::enqueueRequest(request);
            break;
        }
        int x = 0;
        std::cin >> x;;

        // Clean shutdown
        PipelineProcess::shutdown();

        std::this_thread::sleep_for(std::chrono::seconds(10));

        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception in main: " << e.what() << std::endl;
        return 1;
    }
}
