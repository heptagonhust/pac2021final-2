#include "barcodeToPositionMulti.h"

BarcodeToPositionMulti::BarcodeToPositionMulti(Options* opt)
{
	mOptions = opt;
	mOutStream = NULL;
	mZipFile = NULL;
	mWriter = NULL;
	mUnmappedWriter = NULL;
	bool isSeq500 = opt->isSeq500;
	mbpmap = new BarcodePositionMap(opt);
	//barcodeProcessor = new BarcodeProcessor(opt, &mbpmap->bpmap);
	if (!mOptions->transBarcodeToPos.fixedSequence.empty() || !mOptions->transBarcodeToPos.fixedSequenceFile.empty()) {
		fixedFilter = new FixedFilter(opt);
		filterFixedSequence = true;
	}
}

BarcodeToPositionMulti::~BarcodeToPositionMulti()
{
	//if (fixedFilter) {
	//	delete fixedFilter;
	//}
	//unordered_map<uint64, Position*>().swap(misBarcodeMap);
}

bool BarcodeToPositionMulti::process()
{
	initOutput();
	RingBuf<ReadPairPack> pack_ringbufs[mOptions->thread];
	
	std::thread producer(std::bind(&BarcodeToPositionMulti::producerTask, this, &pack_ringbufs[0]));

	Result** results = new Result*[mOptions->thread];
	BarcodeProcessor** barcodeProcessors = new BarcodeProcessor*[mOptions->thread];
	for (int t = 0; t < mOptions->thread; t++) {
		results[t] = new Result(mOptions, true);
		results[t]->setBarcodeProcessor(mbpmap->getBpmap());
	}

	std::thread** threads = new thread * [mOptions->thread];
	for (int t = 0; t < mOptions->thread; t++) {
		threads[t] = new std::thread(std::bind(&BarcodeToPositionMulti::consumerTask, this, t, &pack_ringbufs[t], results[t]));
	}

	std::thread* writerThread = NULL;
	std::thread* unMappedWriterThread = NULL;
	if(mWriter){
		writerThread = new std::thread(std::bind(&BarcodeToPositionMulti::writeTask, this, mWriter));
	}
	if (mUnmappedWriter) {
		unMappedWriterThread = new std::thread(std::bind(&BarcodeToPositionMulti::writeTask, this, mUnmappedWriter));
	}

	producer.join();
	for (int t = 0; t < mOptions->thread; t++) {
		threads[t]->join();
	}

	if (mWriter)
		mWriter->setInputCompleted();
	if (mUnmappedWriter)
		mUnmappedWriter->setInputCompleted();

	if (writerThread)
		writerThread->join();
	if (unMappedWriterThread)
		unMappedWriterThread->join();

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
		delete threads[t];
		threads[t] = NULL;
		delete results[t];
		results[t] = NULL;
	}

	delete[] threads;
	delete[] results;

	if (writerThread)
		delete writerThread;
	if (unMappedWriterThread)
		delete unMappedWriterThread;

	closeOutput();

	return true;
}

void BarcodeToPositionMulti::initOutput() {
	mWriter = new WriterThread(mOptions->out, mOptions->compression);
	if (!mOptions->transBarcodeToPos.unmappedOutFile.empty()) {
		mUnmappedWriter = new WriterThread(mOptions->transBarcodeToPos.unmappedOutFile, mOptions->compression);
	}
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

bool BarcodeToPositionMulti::processPairEnd(ReadPairPack* pack, Result* result)
{
	string outstr;
	string unmappedOut;
	bool hasPosition;
	bool fixedFiltered;
	for (int p = 0; p < pack->count; p++) {
		result->mTotalRead++;
		ReadPair* pair = &pack->data[p];
		Read* or1 = pair->mLeft;
		Read* or2 = pair->mRight;
		if (filterFixedSequence) {
			fixedFiltered = fixedFilter->filter(or1, or2, result);
			if (fixedFiltered) {
				pair->mLeft = NULL;
				pair->mRight = NULL;
				delete or1;
				delete or2;
				continue;
			}
		}
		hasPosition = result->mBarcodeProcessor->process(or1, or2);
		if (hasPosition) {
			outstr += or2->toString();
		}
		else if (mUnmappedWriter) {
			unmappedOut += or2->toString();
		}
		pair->mLeft = NULL;
		pair->mRight = NULL;
		delete or1;
		delete or2;
	}

	mOutputMtx.lock();
	if (mUnmappedWriter && !unmappedOut.empty()) {
		//write reads that can't be mapped to the slide
		mUnmappedWriter->input(unmappedOut.c_str(), unmappedOut.size());
	}
	if (mWriter && !outstr.empty()) {
		mWriter->input(outstr.c_str(), outstr.size());
	}
	mOutputMtx.unlock();

	return true;
}

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

void BarcodeToPositionMulti::consumerTask(int thread_id, RingBuf<ReadPairPack> *rb, Result* result) {
	const int num_thread = mOptions->thread;	
	while (true) {
		ReadPairPack* pack = rb->dequeue_acquire();
		if(pack->count == 0) {
			if (mOptions->verbose) {
				string msg = "finished " + to_string(thread_id) + " threads. Data processing completed.";
				loginfo(msg);
			}
			break;
		} else {
			processPairEnd(pack, result);
		}
		rb->dequeue();
	}

	if (mOptions->verbose) {
		string msg = "finished one thread";
		loginfo(msg);
	}
}

void BarcodeToPositionMulti::writeTask(WriterThread* config) {
	config->outputTask();

	if (mOptions->verbose) {
		string msg = config->getFilename() + " writer finished";
		loginfo(msg);
	}
}
