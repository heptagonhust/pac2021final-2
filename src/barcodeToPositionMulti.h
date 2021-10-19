#ifndef BARCODETOPOSITIONMULTI_H
#define BARCODETOPOSITIONMULTI_H

#include <cstring>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <functional>
#include "options.h"
#include "barcodePositionMap.h"
#include "barcodeProcessor.h"
#include "fixedfilter.h"
#include "writerThread.h"
#include "result.h"

#include "ringbuf.hpp"
#include "ringbuf_pair.h"

using namespace std;

const int max_length_char_buffer = 1ll<<20;

struct BufferedChar {
	char *str;
	size_t length;
	BufferedChar() {
		str = new char[max_length_char_buffer];
		length = 0;
		str[0] = '\0';
	}
	~BufferedChar() {
		// delete [] str;
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
class BarcodeToPositionMulti {
public:
	BarcodeToPositionMulti(Options* opt);
	~BarcodeToPositionMulti();
	bool process();
private:
	void initOutput();
	void closeOutput();
	bool processPairEnd(int thread_id, ReadPairPack* pack, Result* result);
	void initPackRepositoey();
	void destroyPackRepository();
	void producePack(ReadPairPack* pack);
	void consumePack(Result* result);
	void producerTaskLeft(RingBufPair *rb, FastqReader *reader);
	void producerTaskRight(RingBufPair *rb, FastqReader *reader);
	void consumerTask(int thread_id, RingBufPair *rb, Result* result);
	void writeTask(WriterThread* config);
	
public:
	Options* mOptions;
	BarcodePositionMap* mbpmap;
	FixedFilter* fixedFilter;
	//unordered_map<uint64, Position*> misBarcodeMap;

private:
	// std::mutex mOutputMtx;
	ofstream* mOutStream;
	WriterThread* mWriter;
	WriterThread* mUnmappedWriter;
	bool filterFixedSequence = false;
	FastqReader* left_reader;
	FastqReader* right_reader;
	char* mGlobalBufLarge;
};

#endif
