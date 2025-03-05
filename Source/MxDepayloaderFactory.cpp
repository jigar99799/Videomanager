#include "MxDepayloaderFactory.h"
#include "Mx_MediaType.h"
GstElement * CMx_DepayloaderFactory::createDepayloader(const gchar* pipeline_name,const std::string & mediaType)
{
	GstElement* depayloader = nullptr;
    std::cerr << "call  received create depay element\n" << mediaType;
	if		(mediaType == MEDIA_TYPE_VIDEO_H264)
	{
		depayloader = gst_element_factory_make("rtph264depay", "h264-depay");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_H265)
	{
		depayloader = gst_element_factory_make("rtph265depay", "h265-depay");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_MJPEG)
	{
		depayloader = gst_element_factory_make("rtpjpegdepay", "jpeg-depay");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_MP4V)
	{
		depayloader = gst_element_factory_make("rtpmp4vdepay", "mp4v-depay");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_RAW)
	{
		depayloader = gst_element_factory_make("rtpvrawdepay", "raw-depay");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_SV3V)
	{
		depayloader = gst_element_factory_make("rtpsv3vdepay", "sv3v-depay");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_H263)
	{
		depayloader = gst_element_factory_make("rtph263depay", "h263-depay");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_H263P)
	{
		depayloader = gst_element_factory_make("rtph263pdepay", "h263p-depay");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_THEORA)
	{
		depayloader = gst_element_factory_make("rtptheoradepay", "theora-depay");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_VP8)
	{
		depayloader = gst_element_factory_make("rtpvp8depay", "vp8-depay");
	}
	else if (mediaType == MEDIA_TYPE_VIDEO_VP9)
	{
		depayloader = gst_element_factory_make("rtpvp9depay", "vp9-depay");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_AAC)
	{
		depayloader = gst_element_factory_make("rtpmp4adepay", "aac-depay");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_MP3)
	{
		depayloader = gst_element_factory_make("rtpmpadepay", "mp3-depay");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_OPUS)
	{
		depayloader = gst_element_factory_make("rtpopusdepay", "opus-depay");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_ALAW)
	{
		depayloader = gst_element_factory_make("rtpalmadepay", "alaw-depay");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_ULAW)
	{
		depayloader = gst_element_factory_make("rtpulawdepay", "ulaw-depay");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_G729)
	{
		depayloader = gst_element_factory_make("rtpg729depay", "g729-depay");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_AMR)
	{
		depayloader = gst_element_factory_make("rtpamrdepay", "amr-depay");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_AMR_WB)
	{
		depayloader = gst_element_factory_make("rtpamrwbdepay", "amrwb-depay");
	}
	else if (mediaType == MEDIA_TYPE_AUDIO_SPEEX)
	{
		depayloader = gst_element_factory_make("rtpspeexdepay", "speex-depay");
	}

	if (depayloader)
	{
        std::cout <<"Created depayloader element:" << gst_element_get_name(depayloader) ;
		//DBG_PRINT(DBG_MODULE_VPM_G, "%s : Created depayloader element: %s",pipeline_name ,gst_element_get_name(depayloader));
	}
	else
	{
       // std::cout <<"Failed to create depayloader element for media type:%s"<<  pipeline_name ;
		//ERR_PRINT(DBG_MODULE_VPM_G, "%s : Failed to create depayloader element for media type: %s", pipeline_name,mediaType);
	}

	return depayloader;
}

GstElement *CMx_DepayloaderFactory::createDepayloaderforAudio(const gchar *pipeline_name, eAudioCodec audiocodec)
{
	GstElement* depayloader = nullptr;

	if		(eAudioCodec::AUDIO_CODEC_G711_ALAW == audiocodec)
	{
		depayloader = gst_element_factory_make("rtppcmadepay", "audio depay");
	}
	else if(eAudioCodec::AUDIO_CODEC_G711_ULAW == audiocodec)
	{
		depayloader = gst_element_factory_make("rtppcmudepay", "audio depay");
	}
	else if(eAudioCodec::AUDIO_CODE_G726 == audiocodec)
	{
		depayloader = gst_element_factory_make("rtpg726depay", "audio depay");
	}
	
	if (depayloader)
	{
        std::cout <<"Created depayloader element: %s %s"<< pipeline_name ,gst_element_get_name(depayloader) ;
		//DBG_PRINT(DBG_MODULE_VPM_G, "%s : Created depayloader element: %s",pipeline_name ,gst_element_get_name(depayloader));
	}
	else
	{
       // std::cout <<"Failed to create depayloader element for media type:%s"<<  pipeline_name ;
		//ERR_PRINT(DBG_MODULE_VPM_G, "%s : Failed to create depayloader element for media type: %s", pipeline_name,mediaType);
	}

	return depayloader;
}
