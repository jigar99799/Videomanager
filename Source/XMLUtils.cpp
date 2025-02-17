#include "XMLUtils.h"

std::string XMLUtils::CreateXML(const MediaStreamDevice& stDevice){

    try
    {
        // Create a new XML document
        Poco::AutoPtr<Poco::XML::Document> doc = new Poco::XML::Document;
        Poco::XML::Element* root = doc->createElement("MediaStreamDevice");
        doc->appendChild(root);

        // Add DeviceName
        addElement(root, "DeviceName", stDevice.sDeviceName);

        // Serialize Input MediaData
        serializeMediaData(root, "InputMediaData", stDevice.stinputMediaData, doc);

        // Serialize Output MediaData (same as InputMediaData)
        serializeMediaData(root, "OutputMediaData", stDevice.stoutputMediaData, doc);

        // Generate XML string
        std::ostringstream xmlStream;
        Poco::XML::DOMWriter writer;
        writer.setOptions(Poco::XML::XMLWriter::PRETTY_PRINT); // Enable pretty print
        writer.writeNode(xmlStream, doc); // Write the document to the stream
        return xmlStream.str();
    }
    catch (const Poco::Exception& ex)
    {
        std::cerr << "Error creating XML: " << ex.displayText() << std::endl;
        return "";
    }

}

void XMLUtils::serializeMediaData(Poco::XML::Element* parent, const std::string& tagName, const MediaData& mediaData, Poco::AutoPtr<Poco::XML::Document>& doc){

    // Create MediaData element
    Poco::XML::Element* mediaDataElement = doc->createElement(tagName);
    parent->appendChild(mediaDataElement);

    // Add SourceType
    addElement(mediaDataElement, "SourceType", std::to_string(static_cast<int>(mediaData.esourceType)));

    // Add MediaCodec
    Poco::XML::Element* mediaCodecElement = doc->createElement("MediaCodec");
    mediaDataElement->appendChild(mediaCodecElement);
    addElement(mediaCodecElement, "VideoCodec", std::to_string(static_cast<int>(mediaData.stMediaCodec.evideocodec)));
    addElement(mediaCodecElement, "AudioCodec", std::to_string(static_cast<int>(mediaData.stMediaCodec.eaudiocodec)));
    addElement(mediaCodecElement, "AudioSampleRate", std::to_string(static_cast<int>(mediaData.stMediaCodec.eaudioSampleRate)));

    // Add MediaFileSource
    Poco::XML::Element* mediaFileSourceElement = doc->createElement("MediaFileSource");
    mediaDataElement->appendChild(mediaFileSourceElement);
    addElement(mediaFileSourceElement, "ContainerFormat", std::to_string(static_cast<int>(mediaData.stFileSource.econtainerFormat)));

    // Add NetworkStreaming
    Poco::XML::Element* networkStreamingElement = doc->createElement("NetworkStreaming");
    mediaDataElement->appendChild(networkStreamingElement);
    addElement(networkStreamingElement, "StreamingProtocol", std::to_string(static_cast<int>(mediaData.stNetworkStreaming.estreamingProtocol)));
    addElement(networkStreamingElement, "IpAddress", mediaData.stNetworkStreaming.sIpAddress);
    addElement(networkStreamingElement, "Port", std::to_string(mediaData.stNetworkStreaming.iPort));

    // Add StreamingType and Action
    addElement(mediaDataElement, "StreamingType", std::to_string(static_cast<int>(mediaData.estreamingType)));

}

void XMLUtils::ParseXML(const std::string& xmlString, MediaStreamDevice& stDevice){

    try
    {
        // Parse the XML string into a document
        Poco::XML::DOMParser parser;
        Poco::AutoPtr<Poco::XML::Document> doc = parser.parseString(xmlString);
        Poco::XML::Element* root = doc->documentElement();

        // Parse the DeviceName element
        stDevice.sDeviceName = getElementValue(root, "DeviceName");

        // Deserialize Input MediaData
        Poco::XML::Element* inputMediaDataElement = root->getChildElement("InputMediaData");
        if (inputMediaDataElement)
        {
            deserializeMediaData(inputMediaDataElement, stDevice.stinputMediaData);
        }

        // Deserialize Output MediaData
        Poco::XML::Element* outputMediaDataElement = root->getChildElement("OutputMediaData");
        if (outputMediaDataElement)
        {
            deserializeMediaData(outputMediaDataElement, stDevice.stoutputMediaData);
        }
    }
    catch (const Poco::Exception& ex)
    {
        std::cerr << "Error parsing XML: " << ex.displayText() << std::endl;
    }

}

void XMLUtils::deserializeMediaData(Poco::XML::Element* mediaDataElement, MediaData& mediaData){

    // Parse SourceType
    mediaData.esourceType = static_cast<eSourceType>(std::stoi(getElementValue(mediaDataElement, "SourceType")));

    // Parse MediaCodec
    Poco::XML::Element* mediaCodecElement = mediaDataElement->getChildElement("MediaCodec");
    if (mediaCodecElement)
    {
        mediaData.stMediaCodec.evideocodec = static_cast<eVideoCodec>(std::stoi(getElementValue(mediaCodecElement, "VideoCodec")));
        mediaData.stMediaCodec.eaudiocodec = static_cast<eAudioCodec>(std::stoi(getElementValue(mediaCodecElement, "AudioCodec")));
        mediaData.stMediaCodec.eaudioSampleRate = static_cast<eAudioSampleRate>(std::stoi(getElementValue(mediaCodecElement, "AudioSampleRate")));
    }

    // Parse MediaFileSource
    Poco::XML::Element* mediaFileSourceElement = mediaDataElement->getChildElement("MediaFileSource");
    if (mediaFileSourceElement)
    {
        mediaData.stFileSource.econtainerFormat = static_cast<eContainerFormat>(std::stoi(getElementValue(mediaFileSourceElement, "ContainerFormat")));
    }

    // Parse NetworkStreaming
    Poco::XML::Element* networkStreamingElement = mediaDataElement->getChildElement("NetworkStreaming");
    if (networkStreamingElement)
    {
        mediaData.stNetworkStreaming.estreamingProtocol = static_cast<eStreamingProtocol>(std::stoi(getElementValue(networkStreamingElement, "StreamingProtocol")));
        mediaData.stNetworkStreaming.sIpAddress = getElementValue(networkStreamingElement, "IpAddress");
        mediaData.stNetworkStreaming.iPort = std::stoi(getElementValue(networkStreamingElement, "Port"));
    }

    // Parse StreamingType and Action
    mediaData.estreamingType = static_cast<eStreamingType>(std::stoi(getElementValue(mediaDataElement, "StreamingType")));

}

void XMLUtils::addElement(Poco::XML::Element* parent, const std::string& name, const std::string& value){

    Poco::XML::Element* element = parent->ownerDocument()->createElement(name);
    Poco::XML::Text* textNode = parent->ownerDocument()->createTextNode(value);
    element->appendChild(textNode);
    parent->appendChild(element);

}

std::string XMLUtils::getElementValue(Poco::XML::Element* parent, const std::string& name){

    Poco::XML::Element* element = parent->getChildElement(name);
    return (element && element->firstChild()) ? element->firstChild()->getNodeValue() : "";
    
}
