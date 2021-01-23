
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

#ifndef FILEXML_H
#define FILEXML_H

#include <stdint.h>
#include <stdio.h>

#define MAX_TITLE 1024
#define MAX_PROPERTIES 1024
#define MAX_LENGTH 4096


class XMLTag
{
public:
	XMLTag();
	~XMLTag();

	int set_delimiters(char left_delimiter, char right_delimiter);
	int reset_tag();     // clear all structures

	int read_tag(char *input, long &position, long length);

	int title_is(const char *title);        // test against title and return 1 if they match
	char *get_title();
	int get_title(char *value);
	int test_property(char *property, char *value);
	const char *get_property_text(int number);
	int get_property_int(int number);
	float get_property_float(int number);
	char *get_property(const char *property);
	char* get_property(const char *property, char *value);
	int32_t get_property(const char *property, int32_t default_);
	int64_t get_property(const char *property, int64_t default_);
//	int get_property(const char *property, int default_);
	float get_property(const char *property, float default_);
	double get_property(const char *property, double default_);

	int set_title(const char *text);       // set the title field
	int set_property(const char *text, const char *value);
	int set_property(const char *text, int32_t value);
	int set_property(const char *text, int64_t value);
//	int set_property(const char *text, int value);
	int set_property(const char *text, float value);
	int set_property(const char *text, double value);
	int write_tag();

	char tag_title[MAX_TITLE];       // title of this tag

	char *tag_properties[MAX_PROPERTIES];      // list of properties for this tag
	char *tag_property_values[MAX_PROPERTIES];     // values for this tag

	int total_properties;
	int len;         // current size of the string

	char string[MAX_LENGTH];
	char temp_string[32];       // for converting numbers
	char left_delimiter, right_delimiter;
};


class FileXML
{
public:
	FileXML(char left_delimiter = '<', char right_delimiter = '>');
	~FileXML();

	void dump();
	int terminate_string();         // append the terminal 0
	int append_newline();       // append a newline to string
	int append_tag();           // append tag object
	int append_text(const char *text);
// add generic text to the string
	int append_text(const char *text, long len);        
// add generic text to the string which contains <>& characters
 	int encode_text(const char *text);      

// Text array is dynamically allocated and deleted when FileXML is deleted
	char* read_text();         // read text, put it in *output, and return it
	int read_text_until(const char *tag_end, char *output, int max_len);     // store text in output until the tag is reached
	int read_tag();          // read next tag from file, ignoring any text, and put it in tag
	// return 1 on failure

	int write_to_file(const char *filename);           // write the file to disk
	int write_to_file(FILE *file);           // write the file to disk
	int read_from_file(const char *filename, int ignore_error = 0);          // read an entire file from disk
	int read_from_string(char *string);          // read from a string

	int reallocate_string(long new_available);     // change size of string to accommodate new output
	int set_shared_string(char *shared_string, long available);    // force writing to a message buffer
	int rewind();

	char *string;      // string that contains the actual file
	long position;    // current position in string file
	long length;      // length of string file for reading
	long available;    // possible length before reallocation
	int share_string;      // string is shared between this and a message buffer so don't delete

	XMLTag tag;
	long output_length;
	char *output;       // for reading text
	char left_delimiter, right_delimiter;
	char filename[1024];  // Filename used in the last read_from_file or write_to_file
};

#endif
