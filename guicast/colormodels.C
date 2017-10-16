/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
 */

#include "bccmodels.h"
#include "bcsignals.h"
#include "colormodels.h"
#include "language.h"


struct cm_names ColorModels::color_model_names[] =
{
	{ BC_TRANSPARENCY, N_("Transparency") },
	{ BC_RGB888, N_("RGB-8 Bit") },
	{ BC_RGBA8888, N_("RGBA-8 Bit") },
	{ BC_RGB161616, N_("RGB-16 Bit") },
	{ BC_RGBA16161616, N_("RGBA-16 Bit") },
	{ BC_BGR8888, N_("BGRA-8 Bit") },
	{ BC_YUV888, N_("YUV-8 Bit") },
	{ BC_YUVA8888, N_("YUVA-8 Bit") },
	{ BC_YUV161616, N_("YUV-16 Bit") },
	{ BC_YUVA16161616, N_("YUVA-16 Bit") },
	{ BC_RGB_FLOAT, N_("RGB-FLOAT") },
	{ BC_RGBA_FLOAT, N_("RGBA-FLOAT") },
	{ BC_YUV420P, N_("YUV420P") },
	{ BC_YUV422P, N_("YUV422P") },
	{ BC_YUV444P, N_("YUV444P") },
	{ BC_YUV422, N_("YUV422") },
	{ BC_COMPRESSED, N_("Compressed") },
	{ -1, 0 }
};


ColorModels::ColorModels()
{
}


const char *ColorModels::name(int cmodel)
{
	for(int i = 0;; i++)
	{
		if(color_model_names[i].value < 0)
			break;
		if(color_model_names[i].value == cmodel)
			return color_model_names[i].name;
	}
	return "Unknown";
}
