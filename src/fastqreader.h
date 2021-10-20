#ifndef FASTQ_READER_H
#define FASTQ_READER_H

#include <stdio.h>
#include <stdlib.h>
#include "read.h"
#include "igzip/igzip_wrapper.h"
#include "common.h"
#include <iostream>
#include <fstream>
#include "ringbuf.hpp"
#include <thread>
#include <functional>
#include <string_view>
#include "isal_reader.h"

#define FQ_BUF_SIZE (1ll<<28)
#define FQ_BUF_SIZE_ONCE (1<<30)

class FastqReader{
public:
	FastqReader(string filename, char* globalBufLarge, bool hasQuality = true, bool phred64=false);
	~FastqReader();
	bool isZipped();

	// void getBytes(size_t& bytesRead, size_t& bytesTotal);

	//this function is not thread-safe
	//do not call read() of a same FastqReader object from different threads concurrently
	Read* read();
	Read* read(Read* dst);

public:
	static bool isZipFastq(string filename);
	static bool isFastq(string filename);
	// static bool test();

private:
	void init();
	void close();
	string_view getLine();
	// void clearLineBreaks(char* line);
	void readToBufLarge();
	//void stringProcess();

private:
	string mFilename;
	char* mZipFileName;
	FILE* mFile;
	bool mZipped;
	bool mHasQuality;
	bool mPhred64;
	ReaderOutputRB reader_output;
	bool mStdinMode;
	size_t mBufReadLength;
	char *mBufLarge;
	bool mNoLineLeft;
	IsalReader* isal_reader;
	// inter state to read line
	char* decompress_buffer_end = NULL;
	char* line_start = NULL;
};

class FastqReaderPair{
public:
	FastqReaderPair(FastqReader* left, FastqReader* right);
	FastqReaderPair(string leftName, string rightName, bool hasQuality = true, bool phred64 = false, bool interleaved = false);
	~FastqReaderPair();
	ReadPair* read();
	ReadPair* read(ReadPair *dst);
public:
	FastqReader* mLeft;
	FastqReader* mRight;
	bool mInterleaved;
};

#endif
