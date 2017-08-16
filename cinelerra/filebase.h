
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

#ifndef FILEBASE_H
#define FILEBASE_H

#include "asset.inc"
#include "assets.inc"
#include "edit.inc"
#include "guicast.h"
#include "file.inc"
#include "filelist.inc"
#include "overlayframe.inc"
#include "strategies.inc"
#include "vframe.inc"

#include <sys/types.h>

// inherited by every file interpreter
class FileBase
{
public:
	FileBase(Asset *asset, File *file);
	virtual ~FileBase();

	friend class File;
	friend class FileList;
	friend class FrameWriter;

	void reset_parameters();

	virtual void get_parameters(BC_WindowBase *parent_window, 
			Asset *asset, 
			BC_WindowBase **format_window,
			int audio_options,
			int video_options,
			int lock_compressor) {};

	virtual int get_index(char *index_path) { return 1; };
	virtual int check_header() { return 0; };  // Test file to see if it is of this type.
	virtual int reset_parameters_derived() {};
	virtual int read_header() {};     // WAV files for getting header
	virtual int open_file(int rd, int wr) {};
	virtual int close_file();
	virtual int close_file_derived() {};
	void set_dither();
	virtual int seek_end() { return 0; };
	virtual int seek_start() { return 0; };
	virtual int64_t get_video_position() { return 0; };
	virtual int64_t get_audio_position() { return 0; };
	virtual int set_video_position(int64_t x) { return 0; };
	virtual int set_audio_position(int64_t x) { return 0; };

// Subclass should call this to add the base class allocation.
// Only used in read mode.
	virtual int64_t get_memory_usage() { return 0; };

	virtual int write_samples(double **buffer, 
		int64_t len) { return 0; };
	virtual int write_frames(VFrame ***frames, int len) { return 0; };
	virtual int read_compressed_frame(VFrame *buffer) { return 0; };
	virtual int write_compressed_frame(VFrame *buffers) { return 0; };
	virtual int64_t compressed_frame_size() { return 0; };
// Doubles are used to allow resampling
	virtual int read_samples(double *buffer, int64_t len) { return 0; };


	virtual int prefer_samples_float() {return 0;};
	virtual int read_samples_float(float *buffer, int64_t len) { return 0; };

	virtual int read_frame(VFrame *frame) { return 1; };

// Return either the argument or another colormodel which read_frame should
// use.
	virtual int colormodel_supported(int colormodel) { return BC_RGB888; };
// This file can copy compressed frames directly from the asset
	virtual int get_render_strategy(ArrayList<int>* render_strategies) { return VRENDER_VPIXEL; };

protected:
	static int match4(const char *in, const char *out);   // match 4 bytes for a quicktime type

	Asset *asset;
	int wr, rd;
	int dither;
	File *file;
};

#endif
