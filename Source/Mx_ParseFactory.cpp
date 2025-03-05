#include "Mx_ParseFactory.h"
#include "Mx_MediaType.h"
CMx_ParseFactory::CMx_ParseFactory()
{
}


CMx_ParseFactory::~CMx_ParseFactory()
{
}

GstElement * CMx_ParseFactory::createParser(const gchar* pipeline_name, const std::string & mediaType)
{
	GstElement* parser = nullptr;
	if (mediaType == MEDIA_TYPE_VIDEO_H264 || mediaType == MEDIA_TYPE_VIDEO_X_H264)
	{
		parser = gst_element_factory_make("h264parse", "h264-parser");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_H265 || mediaType == MEDIA_TYPE_VIDEO_X_H265)
	{
		parser = gst_element_factory_make("h265parse", "h265-parser");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_MPEG)
	{
		parser = gst_element_factory_make("mpegvideoparse", "mpegvideo-parser");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_VP8)
	{
		parser = gst_element_factory_make("vp8parse", "vp8-parser");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_VP9)
	{
		parser = gst_element_factory_make("vp9parse", "vp9-parser");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_THEORA)
	{
		parser = gst_element_factory_make("theoraparse", "theora-parser");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_MJPEG)
	{
		parser = gst_element_factory_make("jpegparse", "jpeg-parser");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_DV)
	{
		parser = gst_element_factory_make("dvparse", "dv-parser");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_VC1)
	{
		parser = gst_element_factory_make("vc1parse", "vc1-parser");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_AAC)
	{
		parser = gst_element_factory_make("aacparse", "aac-parser");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_MPEG)
	{
		parser = gst_element_factory_make("mpegaudioparse", "mpegaudio-parser");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_AC3)
	{
		parser = gst_element_factory_make("ac3parse", "ac3-parser");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_OPUS)
	{
		parser = gst_element_factory_make("opusparse", "opus-parser");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_FLAC)
	{
		parser = gst_element_factory_make("flacparse", "flac-parser");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_WAV)
	{
		parser = gst_element_factory_make("wavparse", "wav-parser");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_VORBIS)
	{
		parser = gst_element_factory_make("vorbisparse", "vorbis-parser");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_GSM)
	{
		parser = gst_element_factory_make("gsmparse", "gsm-parser");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_SPEEX)
	{
		parser = gst_element_factory_make("speexparse", "speex-parser");
	}
	else
	{
		std::cerr << "Error: Unsupported media type for parser: " << std::endl;
	}

	if (parser)
	{
		//DBG_PRINT(DBG_MODULE_VPM_G, "%s : Created parser element: %s",pipeline_name, gst_element_get_name(parser));
	}
	else
	{
		//ERR_PRINT(DBG_MODULE_VPM_G, "%s : Failed to create parser element for media type: %s", pipeline_name,mediaType);
	}

	return parser;

}
