#include "barcodeToPositionMulti.h"
#include "isal_writer.h"

#define GLOBAL_BUF_SIZE (1ll<<29)

BarcodeToPositionMulti::BarcodeToPositionMulti(Options* opt)
{
	mOptions = opt;
	mOutStream = NULL;
	mWriter = NULL;
	mUnmappedWriter = NULL;
	bool isSeq500 = opt->isSeq500;
	mGlobalBufLarge = new char[GLOBAL_BUF_SIZE];

	// left: [0, FQ_BUF_SIZE), right: [FQ_BUF_SIZE, 2*FQ_BUF_SIZE)
	left_reader = new FastqReader(mOptions->transBarcodeToPos.in1, mGlobalBufLarge);
	right_reader = new FastqReader(mOptions->transBarcodeToPos.in2, mGlobalBufLarge + FQ_BUF_SIZE);

	mbpmap = new BarcodePositionMap(opt);
	//barcodeProcessor = new BarcodeProcessor(opt, &mbpmap->bpmap);
	if (!mOptions->transBarcodeToPos.fixedSequence.empty() || !mOptions->transBarcodeToPos.fixedSequenceFile.empty()) {
		fixedFilter = new FixedFilter(opt);
		filterFixedSequence = true;
	}
}

BarcodeToPositionMulti::~BarcodeToPositionMulti()
{
	delete [] mGlobalBufLarge;
	//if (fixedFilter) {
	//	delete fixedFilter;
	//}
	//unordered_map<uint64, Position*>().swap(misBarcodeMap);
}

bool BarcodeToPositionMulti::process()
{
	RingBufPair pack_ringbufs[mOptions->thread];

	std::thread producerLeft(std::bind(
		&BarcodeToPositionMulti::producerTaskLeft, this, &pack_ringbufs[0], left_reader));
	std::thread producerRight(std::bind(
		&BarcodeToPositionMulti::producerTaskRight, this, &pack_ringbufs[0], right_reader));

	Result** results = new Result*[mOptions->thread];
	BarcodeProcessor** barcodeProcessors = new BarcodeProcessor*[mOptions->thread];
	for (int t = 0; t < mOptions->thread; t++) {
		results[t] = new Result(mOptions, true);
		results[t]->setBarcodeProcessor(mbpmap->getBpmap());
	}

	std::thread** threads = new thread * [mOptions->thread];
	WriterInputRB unmapped_outs[mOptions->thread];
	WriterInputRB mapped_outs[mOptions->thread];
	for (int t = 0; t < mOptions->thread; t++) {
		WriterInputRB *unmapped_out = NULL;
		if(!mOptions->transBarcodeToPos.unmappedOutFile.empty())
			unmapped_out = &unmapped_outs[t];
		threads[t] = new std::thread(std::bind(&BarcodeToPositionMulti::consumerTask, this, t, &pack_ringbufs[t], results[t], &mapped_outs[t], unmapped_out));
	}

	producerLeft.detach();
	producerRight.detach();

	mWriter = new IsalWriter(mOptions, mOptions->out, mapped_outs);
	if (!mOptions->transBarcodeToPos.unmappedOutFile.empty()) {
		mUnmappedWriter = new IsalWriter(mOptions, mOptions->transBarcodeToPos.unmappedOutFile, unmapped_outs);
	}

	if (mWriter)
		mWriter->join();
	if (mUnmappedWriter)
		mUnmappedWriter->join();

	if (mOptions->verbose)
		loginfo("start to generate reports\n");

	//merge result
	vector<Result*> resultList;
	for (int t = 0; t < mOptions->thread; t++){
		resultList.push_back(results[t]);
	}
	Result* finalResult = Result::merge(resultList);
	finalResult->print();

	
	cout << resetiosflags(ios::fixed) << setprecision(2);
	if (!mOptions->transBarcodeToPos.mappedDNBOutFile.empty()) {
		cout << "mapped_dnbs: " << finalResult->mBarcodeProcessor->mDNB.size() << endl;
		finalResult->dumpDNBs(mOptions->transBarcodeToPos.mappedDNBOutFile);
	}
	
	//clean up
	for (int t = 0; t < mOptions->thread; t++) {
		threads[t]->join();
		delete threads[t];
		threads[t] = NULL;
		delete results[t];
		results[t] = NULL;
	}

	delete[] threads;
	delete[] results;

	// if (writerThread)
	// 	delete writerThread;
	// if (unMappedWriterThread)
	// 	delete unMappedWriterThread;

	closeOutput();

	return true;
}

void BarcodeToPositionMulti::closeOutput()
{
	if (mWriter) {
		delete mWriter;
		mWriter = NULL;
	}
	if (mUnmappedWriter) {
		delete mUnmappedWriter;
		mUnmappedWriter = NULL;
	}
}

bool BarcodeToPositionMulti::processPairEnd(ReadPairPack* pack, Result* result, WriterInputRB *mapped_out, WriterInputRB *unmapped_out)
{
	BufferedChar *outstr = new BufferedChar;
	BufferedChar *unmappedOut = new BufferedChar;
	bool hasPosition;
	bool fixedFiltered;
	const int count = min(pack->count_right, pack->count_left);
	for (int p = 0; p < count; p++) {
		result->mTotalRead++;
		IReadPair* pair = &pack->data[p];
		Read* or1 = &pair->mLeft;
		Read* or2 = &pair->mRight;
		if (filterFixedSequence) {
			fixedFiltered = fixedFilter->filter(or1, or2, result);
			if (fixedFiltered) {
				continue;
			}
		}
		hasPosition = result->mBarcodeProcessor->process(or1, or2);
		if (hasPosition) {
			outstr->appendThenFree(or2->toNewCharPointer());
		}
		else if (mUnmappedWriter) {
			unmappedOut->appendThenFree(or2->toNewCharPointer());
		}
	}

	if (mUnmappedWriter && unmappedOut->length != 0) {
		*unmapped_out->enqueue_acquire() = unmappedOut;
		unmapped_out->enqueue();
	}
	if (mWriter && outstr->length != 0) {
		*mapped_out->enqueue_acquire() = outstr;
		mapped_out->enqueue();
	}
	
	return true;
}

void BarcodeToPositionMulti::producerTaskLeft(RingBufPair *rb, FastqReader *reader) {
	if (mOptions->verbose)
		loginfo("start to load data");
	
	bool mInterleaved = false;
	assert(mInterleaved == false);

	int count = 0;

	int thread_id = 0;
	int num_threads = mOptions->thread;
	ReadPairPack* pack = rb[thread_id].enqueue_acquire_left();
	while (true) {
		Read* read = reader->read(&pack->data[count].mLeft);

		if (!read) {
			pack->count_left = count;
			rb[thread_id].enqueue_left();
			break;
		}

		count++;
		
		if (count == PACK_SIZE) {
			pack->count_left = count;
			rb[thread_id].enqueue_left();

			thread_id = (thread_id + 1) % num_threads;
			pack = rb[thread_id].enqueue_acquire_left();

			// reset count to 0
			count = 0;
		}
	}

	// produce empty pack to tell consumer to stop
	for(int i=0;i<mOptions->thread;i++) {
		ReadPairPack* pack = rb[i].enqueue_acquire_left();
		pack->count_left = 0;

		rb[i].enqueue_left();
	}

	if (mOptions->verbose) {
		loginfo("all reads of in1 loaded, start to monitor thread status");
	}
}

void BarcodeToPositionMulti::producerTaskRight(RingBufPair *rb, FastqReader *reader) {
	bool mInterleaved = false;
	assert(mInterleaved == false);

	int count = 0;

	int thread_id = 0;
	int num_threads = mOptions->thread;
	ReadPairPack* pack = rb[thread_id].enqueue_acquire_right();
	while (true) {
		Read* read = reader->read(&pack->data[count].mRight);

		if (!read) {
			pack->count_right = count;
			rb[thread_id].enqueue_right();
			break;
		}
		count++;

		if (count == PACK_SIZE) {
			pack->count_right = count;
			rb[thread_id].enqueue_right();

			thread_id = (thread_id + 1) % num_threads;
			pack = rb[thread_id].enqueue_acquire_right();

			// reset count to 0
			count = 0;
		}
	}

	// produce empty pack to tell consumer to stop
	for(int i=0;i<mOptions->thread;i++) {
		ReadPairPack* pack = rb[i].enqueue_acquire_right();
		pack->count_right = 0;

		rb[i].enqueue_right();
	}

	if (mOptions->verbose) {
		loginfo("all reads of in2 loaded, start to monitor thread status");
	}
}

/*
void BarcodeToPositionMulti::producerTask(RingBuf<ReadPairPack> *rb) {
	if (mOptions->verbose)
		loginfo("start to load data");

	FastqReaderPair reader(mOptions->transBarcodeToPos.in1, mOptions->transBarcodeToPos.in2, true);

	long lastReported = 0;
	long readNum = 0;
	int count = 0;

	int thread_id = 0;
	int num_threads = mOptions->thread;
	ReadPairPack* pack = rb[thread_id].enqueue_acquire();
	while (true) {
		ReadPair* read = reader.read(&pack->data[count]);
		if (!read) {
			pack->count = count;
			rb[thread_id].enqueue();
			break;
		}

		count++;

		// opt report
		if (mOptions->verbose && count + readNum >= lastReported + 1000000) {
			lastReported = count + readNum;
			string msg = "loaded " + to_string((lastReported / 1000000)) + "M read pairs";
			loginfo(msg);
		}

		if (count == PACK_SIZE) {
			pack->count = count;
			rb[thread_id].enqueue();

			thread_id = (thread_id + 1) % num_threads;
			pack = rb[thread_id].enqueue_acquire();

			// reset count to 0
			readNum += count;
			count = 0;
		}
	}

	// produce empty pack to tell consumer to stop
	for(int i=0;i<mOptions->thread;i++) {
		ReadPairPack* pack = rb[i].enqueue_acquire();
		pack->count = 0;

		rb[i].enqueue();
	}

	if (mOptions->verbose) {
		loginfo("all reads loaded, start to monitor thread status");
	}
}
*/

void BarcodeToPositionMulti::consumerTask(int thread_id, RingBufPair *rb, Result* result, WriterInputRB *mapped_out, WriterInputRB *unmapped_out) {
	const int num_thread = mOptions->thread;	
	while (true) {
		ReadPairPack* pack = rb->dequeue_acquire();
		if(pack->count_left == 0 || pack->count_right == 0) {
			if (mOptions->verbose) {
				string msg = "finished " + to_string(thread_id) + " threads. Data processing completed.";
				loginfo(msg);
			}
			break;
		} else {
			processPairEnd(pack, result, mapped_out, unmapped_out);
		}
		rb->dequeue();
	}
	assert(unmapped_out == NULL);

	BufferedChar**in = mapped_out->enqueue_acquire();
	*in = NULL;
	mapped_out->enqueue();
	if(unmapped_out) {
		BufferedChar**in = unmapped_out->enqueue_acquire();
		*in = NULL;
		unmapped_out->enqueue();
	}
	if (mOptions->verbose) {
		string msg = "finished one thread";
		loginfo(msg);
	}
}

void BarcodeToPositionMulti::writeTask(WriterThread* config, RingBuf<BufferedChar*> *writer_out) {
	assert(0); // deprecated
	//config->outputTask(writer_out, mOptions->thread);

	if (mOptions->verbose) {
		string msg = config->getFilename() + " writer finished";
		loginfo(msg);
	}
}
