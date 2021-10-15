#include "fastqreader.h"
#include "util.h"
#include <string.h>

#define FQ_BUF_SIZE (1ll<<28)
#define FQ_BUF_SIZE_ONCE (1<<20)

FastqReader::FastqReader(string filename, bool hasQuality, bool phred64){
	mFilename = filename;
	mZipFile = NULL;
	mZipped = false;
	mFile = NULL;
	mStdinMode = false;
	mPhred64 = phred64;
	mHasQuality = hasQuality;
	mBufLarge = new char[FQ_BUF_SIZE];
	mReadFinished = false;
	mHasNoLineBreakAtEnd = false;
	mStringProcessedLength = 0;
	mNoLineLeftInRingBuf = false;
	produce_rb = new RingBuf<char*>(1ll<<20);
	getlineCounter = 0;
	returnCounter = 0;
	init();
	
}

FastqReader::~FastqReader(){
	close();
	delete mBufLarge;
}

bool FastqReader::hasNoLineBreakAtEnd() {
	return mHasNoLineBreakAtEnd;
}

void FastqReader::readToBufLarge() {
	if(mZipped) {
		size_t mBufDataLen;
		for(mBufReadLength = 0; mBufReadLength < FQ_BUF_SIZE; mBufReadLength += mBufDataLen) {
			mBufDataLen = gzread(mZipFile, mBufLarge+mBufReadLength, FQ_BUF_SIZE_ONCE);
			// stringProcess();
			if(mBufDataLen == 0) {
				break;
			}
			if(mBufDataLen == -1) {
				error_exit("Error to read gzip file: " + mFilename);
				//cerr << "Error to read gzip file" << endl;
			}
		}
	} else {
		size_t mBufDataLen;
		for(mBufReadLength = 0; mBufReadLength < FQ_BUF_SIZE; mBufReadLength += mBufDataLen) {
			mBufDataLen = fread(mBufLarge+mBufReadLength, 1, FQ_BUF_SIZE_ONCE, mFile);
			if(mBufDataLen == 0) {
				break;
			}
			if(mBufDataLen == -1) {
				error_exit("Error to read gzip file: " + mFilename);
				//cerr << "Error to read gzip file" << endl;
			}
		}
	}
	mReadFinished = true;
	stringProcess();
}

void FastqReader::stringProcess() {
	char* ptr = &mBufLarge[0];
	char** rb_item = nullptr;
	// rb_item = produce_rb->enqueue_acquire();
	// *rb_item = &mBufLarge[0];
	// produce_rb->enqueue();
	char *old = nullptr;
	while (!mReadFinished || mStringProcessedLength < mBufReadLength) {
		rb_item = produce_rb->enqueue_acquire();
		*rb_item = ptr;
		produce_rb->enqueue();
		while(*ptr != '\n' && *ptr != '\r' && mStringProcessedLength < mBufReadLength) {
			ptr ++;
			mStringProcessedLength ++;
		}
		while((*ptr == '\n' || *ptr == '\r') && mStringProcessedLength < mBufReadLength) {
			*ptr = '\0';
			ptr ++;
			mStringProcessedLength ++;
		}
	}
	rb_item = produce_rb->enqueue_acquire();
	*rb_item = nullptr;
	produce_rb->enqueue();
}

void FastqReader::init(){
	if (ends_with(mFilename, ".gz")){
		mZipFile = gzopen(mFilename.c_str(), "r");
		mZipped = true;
		gzrewind(mZipFile);
	}
	else {
		if(mFilename == "/dev/stdin") {
			mFile = stdin;
		}
		else
			mFile = fopen(mFilename.c_str(), "rb");
		if(mFile == NULL) {
			error_exit("Failed to open file: " + mFilename);
		}
		mZipped = false;
	}
	std::thread readBuffer(std::bind(&FastqReader::readToBufLarge, this));
	readBuffer.detach();
	//std::thread stringProcess(std::bind(&FastqReader::stringProcess, this));
	//stringProcess.detach();
}

void FastqReader::getBytes(size_t& bytesRead, size_t& bytesTotal) {
	if(mZipped) {
		bytesRead = gzoffset(mZipFile);
	} else {
		bytesRead = ftell(mFile);//mFile.tellg();
	}

	// use another ifstream to not affect current reader
	ifstream is(mFilename);
	is.seekg (0, is.end);
	bytesTotal = is.tellg();
}

void FastqReader::clearLineBreaks(char* line) {

	// trim \n, \r or \r\n in the tail
	int readed = strlen(line);
	if(readed >=2 ){
		if(line[readed-1] == '\n' || line[readed-1] == '\r'){
			line[readed-1] = '\0';
			if(line[readed-2] == '\r')
				line[readed-2] = '\0';
		}
	}
}

string FastqReader::getLine(){
	getlineCounter ++;
	//cout << "in getline: " << getlineCounter << endl;
	char** ptr = produce_rb->dequeue_acquire();
	if(*ptr == nullptr) {
		produce_rb->dequeue();
		mNoLineLeftInRingBuf = true;
		returnCounter ++;
		//cout << "nullptr: " << returnCounter << endl;
		return string();
	} else {
		string ret = string(*ptr, strlen(*ptr));
		produce_rb->dequeue();
		returnCounter++;
		//cout << "return ret: " << returnCounter << endl;
		return ret;
	}
}

bool FastqReader::eof() {
	if (mZipped) {
		return gzeof(mZipFile);
	} else {
		return feof(mFile);//mFile.eof();
	}
}

Read* FastqReader::read(){
	if (mZipped){
		if (mZipFile == NULL)
			return NULL;
	}

	if (mNoLineLeftInRingBuf)
		return NULL;

	string name = getLine();
	// name should start with @
	while((name.empty() && !mNoLineLeftInRingBuf) || (!name.empty() && name[0]!='@')){
		name = getLine();
	}

	if(name.empty())
		return NULL;

	string sequence = getLine();
	string strand = getLine();

	// WAR for FQ with no quality
	if (!mHasQuality){
		string quality = string(sequence.length(), 'K');
		return new Read(name, sequence, strand, quality, mPhred64);
	}
	else {
		string quality = getLine();
		if(quality.length() != sequence.length()) {
			cerr << "ERROR: sequence and quality have different length:" << endl;
			cerr << name << endl;
			cerr << sequence << endl;
			cerr << strand << endl;
			cerr << quality << endl;
			return NULL;
		}
		return new Read(name, sequence, strand, quality, mPhred64);
	}

	return NULL;
}

void FastqReader::close(){
	if (mZipped){
		if (mZipFile){
			gzclose(mZipFile);
			mZipFile = NULL;
		}
	}
	else {
		if (mFile){
			fclose(mFile);//mFile.close();
			mFile = NULL;
		}
	}
}

bool FastqReader::isZipFastq(string filename) {
	if (ends_with(filename, ".fastq.gz"))
		return true;
	else if (ends_with(filename, ".fq.gz"))
		return true;
	else if (ends_with(filename, ".fasta.gz"))
		return true;
	else if (ends_with(filename, ".fa.gz"))
		return true;
	else
		return false;
}

bool FastqReader::isFastq(string filename) {
	if (ends_with(filename, ".fastq"))
		return true;
	else if (ends_with(filename, ".fq"))
		return true;
	else if (ends_with(filename, ".fasta"))
		return true;
	else if (ends_with(filename, ".fa"))
		return true;
	else
		return false;
}

bool FastqReader::isZipped(){
	return mZipped;
}

bool FastqReader::test(){
	FastqReader reader1("testdata/R1.fq");
	FastqReader reader2("testdata/R1.fq.gz");
	Read* r1 = NULL;
	Read* r2 = NULL;
	while(true){
		r1=reader1.read();
		r2=reader2.read();
		if(r1 == NULL || r2 == NULL)
			break;
		if(r1->mSeq.mStr != r2->mSeq.mStr){
			return false;
		}
		delete r1;
		delete r2;
	}
	return true;
}

FastqReaderPair::FastqReaderPair(FastqReader* left, FastqReader* right){
	mLeft = left;
	mRight = right;
}

FastqReaderPair::FastqReaderPair(string leftName, string rightName, bool hasQuality, bool phred64, bool interleaved){
	mInterleaved = interleaved;
	mLeft = new FastqReader(leftName, hasQuality, phred64);
	if(mInterleaved)
		mRight = NULL;
	else
		mRight = new FastqReader(rightName, hasQuality, phred64);
}

FastqReaderPair::~FastqReaderPair(){
	if(mLeft){
		delete mLeft;
		mLeft = NULL;
	}
	if(mRight){
		delete mRight;
		mRight = NULL;
	}
}

ReadPair* FastqReaderPair::read(){
	Read* l = mLeft->read();
	Read* r = NULL;
	if(mInterleaved)
		r = mLeft->read();
	else
		r = mRight->read();
	if(!l || !r){
		return NULL;
	} else {
		return new ReadPair(l, r);
	}
}

ReadPair* FastqReaderPair::read(ReadPair *dst){
	Read* l = mLeft->read();
	Read* r = NULL;
	if(mInterleaved)
		r = mLeft->read();
	else
		r = mRight->read();
	if(!l || !r){
		return NULL;
	} else {
		dst->mLeft = l;
		dst->mRight = r;
		return dst;
	}
}

