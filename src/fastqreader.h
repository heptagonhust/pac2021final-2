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

#define FQ_BUF_SIZE (1ll<<35)
#define FQ_BUF_SIZE_ONCE (1<<29)


#define LINE_PACK_SIZE 4096
struct LinePack {
	string_view lines[LINE_PACK_SIZE];
	int size;
	LinePack() : size(0) {}
};


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
	// bool eof();
	bool hasNoLineBreakAtEnd();

public:
	static bool isZipFastq(string filename);
	static bool isFastq(string filename);
	// static bool test();

private:
	void init();
	void close();
	const string_view& getLine();
	// void clearLineBreaks(char* line);
	void readToBufLarge();
	void stringProcess();

private:
	string mFilename;
	char* mZipFileName;
	FILE* mFile;
	bool mZipped;
	bool mHasQuality;
	bool mPhred64;
	RingBuf<LinePack> line_ptr_rb;
	RingBuf<size_t> input_buffer_rb;
	bool mStdinMode;
	bool mHasNoLineBreakAtEnd;
	size_t mBufReadLength;
	size_t mStringProcessedLength;
	char *mBufLarge;
	bool mNoLineLeftInRingBuf;

	LinePack* line_pack_outputing = nullptr;
	size_t line_pack_n_outputed = 0;
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
