#pragma once
#include <gst/gst.h>
#include <string>
#include <iostream>
#include "Struct.h"

class CMx_DepayloaderFactory 
{
	
public:
	// This method will return the appropriate depayloader element based on the media type.
	static GstElement* createDepayloader(const gchar* pipeline_name,const std::string& mediaType);

	//Overload function create
	static GstElement* createDepayloaderforAudio(const gchar* pipeline_name,eAudioCodec audiocodec);

};