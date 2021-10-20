#ifndef WRITER_THREAD_H
#define WRITER_THREAD_H

#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>
#include "writer.h"
#include <atomic>
#include <mutex>
#include "ringbuf.hpp"

using namespace std;
// constexpr size_t WRITER_OUTPUT_PACK_SIZE = 512;
// struct OutputStrPack {
// 	char data[WRITER_OUTPUT_PACK_SIZE];
// 	size_t size;
// 	OutputStrPack() {
// 		size = 0;
// 	}
// };

struct StringList {
	char* data;
	size_t data_size;
	struct StringList* next;

	StringList() {
		data = NULL;
		data_size = 0;
		next = NULL;
	}
	StringList(char *str, size_t size, StringList* nx) {
		data = str;
		data_size = size;
		next = nx;
	}
	void append(char *str, size_t size) {
		next = new StringList(str, size, next);
	}
};

class WriterThread {
public:
	WriterThread(string filename, int compressionLevel = 4);
	~WriterThread();

	void initWriter(string filename1);
	void initWriter(ofstream* stream);
	// void initWriter(gzFile gzfile);

	void cleanup();

	bool isCompleted();
	void outputTask();
	void input(int thread_id, const char* data, size_t size);
	size_t setInputCompleted(int thread_num, char* bufLarge);

	long bufferLength();
	string getFilename() { return mFilename; }

private:
	void deleteWriter();

private:
	Writer* mWriter1;
	int compression;
	string mFilename;

	//for split output
	bool mInputCompleted;
	// RingBuf<OutputStrPack> rb;
	char* bufferToCompress;
	size_t bufferTotalSize;

	StringList headStrList[64];
};

#endif