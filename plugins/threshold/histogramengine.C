#include "histogramengine.h"
#include "../colors/plugincolors.h"
#include "vframe.h"

#include <stdio.h>
#include <string.h>

HistogramPackage::HistogramPackage()
 : LoadPackage()
{
	start = end = 0;
}


HistogramUnit::HistogramUnit(HistogramEngine *server)
 : LoadClient(server)
{
	for(int i = 0; i < 5; i++)
		accum[i] = new int64_t[HISTOGRAM_RANGE];
}

HistogramUnit::~HistogramUnit()
{
	for(int i = 0; i < 5; i++)
		delete [] accum[i];
}

void HistogramUnit::process_package(LoadPackage *package)
{
	HistogramPackage *pkg = (HistogramPackage*)package;
	HistogramEngine *server = (HistogramEngine*)get_server();


#define HISTOGRAM_HEAD(type) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)data->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{

#define HISTOGRAM_TAIL(components) \
			v = (r * 76 + g * 150 + b * 29) >> 8; \
/*			v = MAX(r, g); */ \
/*			v = MAX(v, b); */ \
			r += -(int)(HISTOGRAM_MIN * 0xffff); \
			g += -(int)(HISTOGRAM_MIN * 0xffff); \
			b += -(int)(HISTOGRAM_MIN * 0xffff); \
			v += -(int)(HISTOGRAM_MIN * 0xffff); \
			CLAMP(r, 0, HISTOGRAM_RANGE); \
			CLAMP(g, 0, HISTOGRAM_RANGE); \
			CLAMP(b, 0, HISTOGRAM_RANGE); \
			CLAMP(v, 0, HISTOGRAM_RANGE); \
			accum_r[r]++; \
			accum_g[g]++; \
			accum_b[b]++; \
			accum_v[v]++; \
/*			if(components == 4) accum_a[row[3]]++; */ \
			row += components; \
		} \
	} \
}



	VFrame *data = server->data;

	int w = data->get_w();
	int h = data->get_h();
	int64_t *accum_r = accum[HISTOGRAM_RED];
	int64_t *accum_g = accum[HISTOGRAM_GREEN];
	int64_t *accum_b = accum[HISTOGRAM_BLUE];
	int64_t *accum_a = accum[HISTOGRAM_ALPHA];
	int64_t *accum_v = accum[HISTOGRAM_VALUE];
	int r, g, b, a, y, u, v;

	switch(data->get_color_model())
	{
		case BC_RGB888:
			HISTOGRAM_HEAD(unsigned char)
			r = (row[0] << 8) | row[0];
			g = (row[1] << 8) | row[1];
			b = (row[2] << 8) | row[2];
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGB_FLOAT:
			HISTOGRAM_HEAD(float)
			r = (int)(row[0] * 0xffff);
			g = (int)(row[1] * 0xffff);
			b = (int)(row[2] * 0xffff);
			HISTOGRAM_TAIL(3)
			break;
		case BC_YUV888:
			HISTOGRAM_HEAD(unsigned char)
			y = (row[0] << 8) | row[0];
			u = (row[1] << 8) | row[1];
			v = (row[2] << 8) | row[2];
			server->yuv->yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGBA8888:
			HISTOGRAM_HEAD(unsigned char)
			r = (row[0] << 8) | row[0];
			g = (row[1] << 8) | row[1];
			b = (row[2] << 8) | row[2];
			HISTOGRAM_TAIL(4)
			break;
		case BC_RGBA_FLOAT:
			HISTOGRAM_HEAD(float)
			r = (int)(row[0] * 0xffff);
			g = (int)(row[1] * 0xffff);
			b = (int)(row[2] * 0xffff);
			HISTOGRAM_TAIL(4)
			break;
		case BC_YUVA8888:
			HISTOGRAM_HEAD(unsigned char)
			y = (row[0] << 8) | row[0];
			u = (row[1] << 8) | row[1];
			v = (row[2] << 8) | row[2];
			server->yuv->yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(4)
			break;
		case BC_RGB161616:
			HISTOGRAM_HEAD(uint16_t)
			r = row[0];
			g = row[1];
			b = row[2];
			HISTOGRAM_TAIL(3)
			break;
		case BC_YUV161616:
			HISTOGRAM_HEAD(uint16_t)
			y = row[0];
			u = row[1];
			v = row[2];
			server->yuv->yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGBA16161616:
			HISTOGRAM_HEAD(uint16_t)
			r = row[0];
			g = row[1];
			b = row[2];
			HISTOGRAM_TAIL(3);
			break;
		case BC_YUVA16161616:
			HISTOGRAM_HEAD(uint16_t)
			y = row[0];
			u = row[1];
			v = row[2];
			server->yuv->yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(4)
			break;
	}
}



	




HistogramEngine::HistogramEngine(int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
{
	yuv = new YUV;
	data = 0;
	for(int i = 0; i < 5; i++)
		accum[i] = new int64_t[HISTOGRAM_RANGE];
}

HistogramEngine::~HistogramEngine()
{
	delete yuv;
	for(int i = 0; i < 5; i++)
		delete [] accum[i];
}

void HistogramEngine::process_packages(VFrame *data)
{
	this->data = data;
	LoadServer::process_packages();
	for(int i = 0; i < 5; i++)
	{
		bzero(accum[i], sizeof(int64_t) * HISTOGRAM_RANGE);
	}

	for(int i = 0; i < get_total_clients(); i++)
	{
		HistogramUnit *unit = (HistogramUnit*)get_client(i);
		for(int k = 0; k < 5; k++)
		{
			for(int j = 0; j < HISTOGRAM_RANGE; j++)
			{
				accum[k][j] += unit->accum[k][j];
			}
		}
	}

}

void HistogramEngine::init_packages()
{
	int total_size = data->get_h();
	int package_size = (int)((float)total_size / 
		get_total_packages() + 1);
	int start = 0;
	for(int i = 0; i < get_total_packages(); i++)
	{
		HistogramPackage *package = (HistogramPackage*)get_package(i);
		package->start = start;
		package->end = start + package_size;
		if(package->end > total_size)
			package->end = total_size;
		start = package->end;
	}

// Initialize clients here in case some don't get run.
	for(int i = 0; i < get_total_clients(); i++)
	{
		HistogramUnit *unit = (HistogramUnit*)get_client(i);
		for(int i = 0; i < 5; i++)
			bzero(unit->accum[i], sizeof(int64_t) * HISTOGRAM_RANGE);
	}
}

LoadClient* HistogramEngine::new_client()
{
	return (LoadClient*)new HistogramUnit(this);
}

LoadPackage* HistogramEngine::new_package()
{
	return (LoadPackage*)new HistogramPackage;
}
