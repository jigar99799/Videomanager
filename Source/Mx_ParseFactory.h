#include <iostream>
#include <gst/gst.h>
class CMx_ParseFactory
{
public:
	CMx_ParseFactory();
	~CMx_ParseFactory();
	static GstElement* createParser(const gchar* pipeline_name,const std::string& mediaType);
};

