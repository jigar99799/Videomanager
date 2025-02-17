#pragma once
#include "Struct.h"
#include <iostream>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <Poco/Dynamic/Var.h>

class JSONUtils {
public:
    // Create JSON string from MediaStreamDevice object
    static std::string CreateJSON(const MediaStreamDevice& device);

    // Parse JSON string into MediaStreamDevice object
    static void ParseJSON(const std::string& jsonString, MediaStreamDevice& device);

private:
    // Helper function to add MediaData to JSON
    static void addMediaDataToJSON(Poco::JSON::Object::Ptr& mediaDataObject, const MediaData& mediaData);

    // Helper function to parse MediaData from JSON
    static void parseMediaDataFromJSON(Poco::JSON::Object::Ptr mediaDataObject, MediaData& mediaData);
};