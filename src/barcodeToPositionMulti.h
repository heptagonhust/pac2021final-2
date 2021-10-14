#ifndef BARCODETOPOSITIONMULTI_H
#define BARCODETOPOSITIONMULTI_H

#include <string>
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

using namespace std;

struct ReadPairPack {
	ReadPair data[PACK_SIZE];
	int count;
	ReadPairPack() {
		count = 0;
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
	bool processPairEnd(ReadPairPack* pack, Result* result);
	void initPackRepositoey();
	void destroyPackRepository();
	void producePack(ReadPairPack* pack);
	void consumePack(Result* result);
	void producerTask(RingBuf<ReadPairPack> *rb);
	void consumerTask(int thread_id, RingBuf<ReadPairPack> *rb, Result* result);
	void writeTask(WriterThread* config);
	
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
};

#endif
