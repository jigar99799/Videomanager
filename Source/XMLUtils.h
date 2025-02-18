#pragma once
#include "Struct.h"
#include <sstream>
#include <Poco/DOM/Document.h>
#include <Poco/AutoPtr.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/DOMParser.h>

class XMLUtils
{
public:
    static std::string CreateXML(const MediaStreamDevice& stDevice);
    static void ParseXML(const std::string& xmlString, MediaStreamDevice& stDevice);

private:
    static void serializeMediaData(Poco::XML::Element* parent, const std::string& tagName, const MediaData& mediaData, Poco::AutoPtr<Poco::XML::Document>& doc);
    static void deserializeMediaData(Poco::XML::Element* mediaDataElement, MediaData& mediaData);
    static void addElement(Poco::XML::Element* parent, const std::string& name, const std::string& value);
    static std::string getElementValue(Poco::XML::Element* parent, const std::string& name);
};