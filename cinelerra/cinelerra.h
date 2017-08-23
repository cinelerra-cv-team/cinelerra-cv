

/*
 * CINELERRA
 * Copyright (C) 2017 Einar Rünkaru <einarry@smail.ee>
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

#ifndef CINELERRA_H
#define CINELERRA_H

// Frame size limits
#define MIN_FRAME_HEIGHT 16
#define MAX_FRAME_HEIGHT 4096
#define MIN_FRAME_WIDTH 16
#define MAX_FRAME_WIDTH 4096

// Samplerate limits
#define MIN_SAMPLE_RATE 8000
#define MAX_SAMPLE_RATE 192000

// Framerate limits
#define MIN_FRAME_RATE 1
#define MAX_FRAME_RATE 1500

// Audio sample formats (must be bits)
#define SBITS_LINEAR8    0x0001
#define SBITS_LINEAR16   0x0002
#define SBITS_LINEAR24   0x0004
#define SBITS_LINEAR32   0x0008
#define SBITS_ULAW       0x0100
#define SBITS_IMA4       0x0200
#define SBITS_ADPCM      0x0400
#define SBITS_FLOAT      0x0800

#define SBITS_LINEAR    (SBITS_LINEAR8 | SBITS_LINEAR16 | SBITS_LINEAR24 | SBITS_LINEAR32)

#endif
