#ifndef _WRITER_H
#define _WRITER_H

#include <stdio.h>
#include <stdlib.h>
#include "igzip/igzip_wrapper.h"
#include "common.h"
#include "ringbuf.hpp"
#include <iostream>
#include <fstream>

using namespace std;

const int max_length_char_buffer = 1ll<<20;

class Writer{
public:
	Writer(string filename, int compression = 3);
	Writer(ofstream* stream);
	// Writer(gzFile gzfile);
	~Writer();
	bool isZipped();
	// bool writeString(string& s);
	// bool writeLine(string& linestr);
	bool write(char* strdata, size_t size);
	string filename();

public:
	static bool test();

private:
	void init();
	void close();

private:
	string mFilename;
	char* mZipFileName;
	ofstream* mOutStream;
	bool mZipped;
	int mCompression;
	bool haveToClose;
};

struct BufferedChar {
	char *str;
	size_t length;
	BufferedChar() {
		str = new char[max_length_char_buffer];
		length = 0;
		str[0] = '\0';
	}
	~BufferedChar() {
		delete [] str;
	}
	void appendThenFree(const char * source) {
		char *dest = str + length;
		const char *toFree = source;
    	while (*source != '\0')
    	{
			length++;
			assert(length < max_length_char_buffer);
        	*dest = *source;
        	dest++;
        	source++;
    	}
    	*dest = '\0';
		delete [] toFree;
	}
};

#endif
