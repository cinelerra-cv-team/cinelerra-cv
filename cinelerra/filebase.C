
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "asset.h"
#include "assets.h"
#include "byteorder.h"
#include "file.h"
#include "filebase.h"
#include "overlayframe.h"

#include <stdlib.h>

FileBase::FileBase(Asset *asset, File *file)
{
	this->file = file;
	this->asset = asset;
	reset_parameters();
}

FileBase::~FileBase()
{
}

int FileBase::close_file()
{
	close_file_derived();
	reset_parameters();
	return 0;
}

void FileBase::set_dither()
{
	dither = 1;
}

void FileBase::reset_parameters()
{
	dither = 0;
	rd = wr = 0;

	reset_parameters_derived();
}

int FileBase::match4(const char *in, const char *out)
{
	if(in[0] == out[0] &&
		in[1] == out[1] &&
		in[2] == out[2] &&
		in[3] == out[3])
		return 1;
	else
		return 0;
}
