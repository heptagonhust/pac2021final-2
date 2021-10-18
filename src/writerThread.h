#ifndef WRITER_THREAD_H
#define WRITER_THREAD_H

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
constexpr size_t WRITER_OUTPUT_PACK_SIZE = 512;
struct OutputStrPack {
	char data[WRITER_OUTPUT_PACK_SIZE];
	size_t size;
	OutputStrPack() {
		size = 0;
	}
};

class WriterThread {
public:
	WriterThread(string filename, int compressionLevel = 4);
	~WriterThread();

	void initWriter(string filename1);
	void initWriter(ofstream* stream);
	void initWriter(gzFile gzfile);

	void cleanup();

	bool isCompleted();
	void outputTask(RingbufWriter *writer_out, int threadNum);
	void input(const char* data, size_t size);
	bool setInputCompleted();

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
	RingBuf<OutputStrPack> rb;

	mutex mtx;
};

#endif