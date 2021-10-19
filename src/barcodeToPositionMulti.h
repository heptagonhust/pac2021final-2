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




class BarcodeToPositionMulti {
public:
	BarcodeToPositionMulti(Options* opt);
	~BarcodeToPositionMulti();
	bool process();
private:
	void initOutput();
	void closeOutput();
	bool processPairEnd(ReadPairPack* pack, Result* result, RingBuf<BufferedChar*> *mapped_out, RingBuf<BufferedChar*> *unmapped_out);
	void initPackRepositoey();
	void destroyPackRepository();
	void producePack(ReadPairPack* pack);
	void consumePack(Result* result);
	void producerTaskLeft(RingBufPair *rb, FastqReader *reader);
	void producerTaskRight(RingBufPair *rb, FastqReader *reader);
	void consumerTask(int thread_id, RingBufPair *rb, Result* result, RingBuf<BufferedChar*> *mapped_out, RingBuf<BufferedChar*> *unmapped_out);
	void writeTask(WriterThread* config, RingBuf<BufferedChar*> *writer_out);
	
public:
	Options* mOptions;
	BarcodePositionMap* mbpmap;
	FixedFilter* fixedFilter;
	//unordered_map<uint64, Position*> misBarcodeMap;

private:
	std::mutex mOutputMtx;
	gzFile mZipFile;
	ofstream* mOutStream;
	WriterThread* mWriter;
	WriterThread* mUnmappedWriter;
	bool filterFixedSequence = false;
	FastqReader* left_reader;
	FastqReader* right_reader;
};

#endif
