#pragma once
enum class eAction {
	ACTION_NONE =	0,
	ACTION_CREATE,
	ACTION_TERMINATE,
	ACTION_PAUSE,
	ACTION_START,
	ACTION_STOP,
	ACTION_UPDATE
};

enum class eSourceType {
	SOURCE_TYPE_NONE =	0,
	SOURCE_TYPE_FILE,
	SOURCE_TYPE_NETWORK,
	SOURCE_TYPE_DISPLAY //temporary display
};

enum class eVideoCodec {
	VIDEO_CODEC_NONE =	0, 
	VIDEO_CODEC_H264,
	VIDEO_CODEC_H265,
	VIDEO_CODEC_VP8,
	VIDEO_CODEC_VP9,
	VIDEO_CODEC_AV1,
	VIDEO_CODEC_MPEG2,
	VIDEO_CODEC_MPEG4
};

enum class eAudioCodec {
	AUDIO_CODEC_NONE =	0,
	AUDIO_CODEC_AAC,
	AUDIO_CODEC_MP3,
	AUDIO_CODEC_OPUS,
	AUDIO_CODEC_FLAC,
	AUDIO_CODEC_ALAC,
	AUDIO_CODEC_WMA
};

enum class eAudioSampleRate {
	AUDIO_SAMPLE_RATE_NONE =	0,
	AUDIO_SAMPLE_RATE_K44_1,
	AUDIO_SAMPLE_RATE_K48
};

enum class eStreamingProtocol {
	STREAMING_PROTOCOL_NONE	=	0,
	STREAMING_PROTOCOL_RTP ,
	STREAMING_PROTOCOL_RTSP,
	STREAMING_PROTOCOL_HTTP,
	STREAMING_PROTOCOL_HTTPS,
	STREAMING_PROTOCOL_WEBRTC,
	STREAMING_PROTOCOL_RTMP
};

enum class eStreamingType {
	STREAMING_TYPE_NONE	=	0,
	STREAMING_TYPE_AUDIO,
	STREAMING_TYPE_VIDEO,
	STREAMING_TYPE_AUDIO_VIDEO,
	STREAMING_TYPE_AUDIO_VIDEO_SUBTITLE,
	STREAMING_TYPE_VIDEO_SUBTITLE
};

enum class eContainerFormat
{
	CONTAINER_FORMAT_NONE	=	0,
	CONTAINER_FORMAT_MP4,
	CONTAINER_FORMAT_MKV,
	CONTAINER_FORMAT_AVI,
	CONTAINER_FORMAT_MOV,
	CONTAINER_FORMAT_WebM
};