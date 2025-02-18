#include "JSONUtils.h"
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <sstream>

std::string JSONUtils::CreateJSON(const MediaStreamDevice& device) 
{
    try 
    {
        Poco::JSON::Object::Ptr root = new Poco::JSON::Object;
        
        // Set DeviceName
        root->set("DeviceName", device.sDeviceName);

        // Create MediaData Object for input and output
        Poco::JSON::Object::Ptr inputMediaData = new Poco::JSON::Object;
        Poco::JSON::Object::Ptr outputMediaData = new Poco::JSON::Object;

        // Add input media data attributes
        addMediaDataToJSON(inputMediaData, device.stinputMediaData);
        // Add output media data attributes
        addMediaDataToJSON(outputMediaData, device.stoutputMediaData);

        // Add MediaData objects to root
        root->set("InputMediaData", inputMediaData);
        root->set("OutputMediaData", outputMediaData);

        // Create JSON string
        std::stringstream jsonStream;
        root->stringify(jsonStream, 2); // Pretty print with indentation of 2
        return jsonStream.str();
    } 
    catch (const Poco::Exception& ex) 
    {
        std::cerr << "Error creating JSON: " << ex.displayText() << std::endl;
        return "";
    }
}

void JSONUtils::addMediaDataToJSON(Poco::JSON::Object::Ptr& mediaDataObject, const MediaData& mediaData) 
{
    mediaDataObject->set("SourceType", static_cast<int>(mediaData.esourceType));

    // MediaCodec
    Poco::JSON::Object::Ptr mediaCodecObject = new Poco::JSON::Object;
    mediaCodecObject->set("VideoCodec", static_cast<int>(mediaData.stMediaCodec.evideocodec));
    mediaCodecObject->set("AudioCodec", static_cast<int>(mediaData.stMediaCodec.eaudiocodec));
    mediaCodecObject->set("AudioSampleRate", static_cast<int>(mediaData.stMediaCodec.eaudioSampleRate));
    mediaDataObject->set("MediaCodec", mediaCodecObject);

    // MediaFileSource
    Poco::JSON::Object::Ptr mediaFileSourceObject = new Poco::JSON::Object;
    mediaFileSourceObject->set("ContainerFormat", static_cast<int>(mediaData.stFileSource.econtainerFormat));
    mediaDataObject->set("MediaFileSource", mediaFileSourceObject);

    // NetworkStreaming
    Poco::JSON::Object::Ptr networkStreamingObject = new Poco::JSON::Object;
    networkStreamingObject->set("StreamingProtocol", static_cast<int>(mediaData.stNetworkStreaming.estreamingProtocol));
    networkStreamingObject->set("IpAddress", mediaData.stNetworkStreaming.sIpAddress);
    networkStreamingObject->set("Port", mediaData.stNetworkStreaming.iPort);
    mediaDataObject->set("NetworkStreaming", networkStreamingObject);

    // StreamingType and Action
    mediaDataObject->set("StreamingType", static_cast<int>(mediaData.estreamingType));
}

void JSONUtils::ParseJSON(const std::string& jsonString, MediaStreamDevice& device) 
{
    try {
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var result = parser.parse(jsonString);
        Poco::JSON::Object::Ptr jsonObject = result.extract<Poco::JSON::Object::Ptr>();

        // Parse DeviceName
        device.sDeviceName = jsonObject->getValue<std::string>("DeviceName");

        // Parse input and output MediaData
        if (jsonObject->has("InputMediaData")) {
            parseMediaDataFromJSON(jsonObject->getObject("InputMediaData"), device.stinputMediaData);
        }
        if (jsonObject->has("OutputMediaData")) {
            parseMediaDataFromJSON(jsonObject->getObject("OutputMediaData"), device.stoutputMediaData);
        }

    } catch (const Poco::Exception& ex) {
        std::cerr << "Error parsing JSON: " << ex.displayText() << std::endl;
    }
}

void JSONUtils::parseMediaDataFromJSON(Poco::JSON::Object::Ptr mediaDataObject, MediaData& mediaData) 
{
    mediaData.esourceType = static_cast<eSourceType>(mediaDataObject->getValue<int>("SourceType"));

    // MediaCodec
    if (mediaDataObject->has("MediaCodec")) 
    {
        Poco::JSON::Object::Ptr mediaCodecObject = mediaDataObject->getObject("MediaCodec");
        mediaData.stMediaCodec.evideocodec = static_cast<eVideoCodec>(mediaCodecObject->getValue<int>("VideoCodec"));
        mediaData.stMediaCodec.eaudiocodec = static_cast<eAudioCodec>(mediaCodecObject->getValue<int>("AudioCodec"));
        mediaData.stMediaCodec.eaudioSampleRate = static_cast<eAudioSampleRate>(mediaCodecObject->getValue<int>("AudioSampleRate"));
    }

    // MediaFileSource
    if (mediaDataObject->has("MediaFileSource")) 
    {
        Poco::JSON::Object::Ptr mediaFileSourceObject = mediaDataObject->getObject("MediaFileSource");
        mediaData.stFileSource.econtainerFormat = static_cast<eContainerFormat>(mediaFileSourceObject->getValue<int>("ContainerFormat"));
    }

    // NetworkStreaming
    if (mediaDataObject->has("NetworkStreaming")) 
    {
        Poco::JSON::Object::Ptr networkStreamingObject = mediaDataObject->getObject("NetworkStreaming");
        mediaData.stNetworkStreaming.estreamingProtocol = static_cast<eStreamingProtocol>(networkStreamingObject->getValue<int>("StreamingProtocol"));
        mediaData.stNetworkStreaming.sIpAddress = networkStreamingObject->getValue<std::string>("IpAddress");
        mediaData.stNetworkStreaming.iPort = networkStreamingObject->getValue<int>("Port");
    }

    // StreamingType
    mediaData.estreamingType = static_cast<eStreamingType>(mediaDataObject->getValue<int>("StreamingType"));
}
