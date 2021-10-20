#include "fastqreader.h"
#include "igzip/igzip_wrapper.h"
#include "util.h"
#include <string.h>
#include <string_view>


FastqReader::FastqReader(string filename, char *globalBufLarge, bool hasQuality, bool phred64)
	: line_ptr_rb(1024), input_buffer_rb(4096 * 1024)
{
	mFilename = filename;
	mZipFileName = NULL;
	mZipped = false;
	mFile = NULL;
	mStdinMode = false;
	mPhred64 = phred64;
	mHasQuality = hasQuality;
	mBufLarge = globalBufLarge;
	mBufReadLength = 0;
	mHasNoLineBreakAtEnd = false;
	mStringProcessedLength = 1;

	init();
}

FastqReader::~FastqReader(){
	close();
}

bool FastqReader::hasNoLineBreakAtEnd() {
	return mHasNoLineBreakAtEnd;
}

void FastqReader::readToBufLarge() {
	if(mZipped) {
		// mBufLarge may overflow, take care!
		if (decompress_file(mZipFileName, (unsigned char*)mBufLarge, &mBufReadLength) != 0) {
			error_exit("Error to read gzip file: " + mFilename);
		}
		if (mBufReadLength >= FQ_BUF_SIZE) {
			assert(false);
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
	stringProcess();
}

void FastqReader::stringProcess() {
	LinePack* line_pack = line_ptr_rb.enqueue_acquire();
	size_t line_pack_size = 0;

	char *line_end, *line_start;
	size_t buff_size = mBufReadLength;
	line_start = line_end = mBufLarge;

	while(true) {
		// find end of line
		line_end = line_start;
		int line_len = 0;
		while(*line_end != '\n' && *line_end != '\r' && *line_end != '\0') {
			line_len++, line_end++;
		}

		// output string view
		buff_size -= line_len;
		// *line_end = '\0';
		line_pack->lines[line_pack_size] = string_view(line_start, line_len);
		line_pack_size++;

		if(line_pack_size >= LINE_PACK_SIZE) {
			line_pack->size = line_pack_size;
			line_ptr_rb.enqueue();

			line_pack = line_ptr_rb.enqueue_acquire();
			line_pack_size = 0;
		}

 		// case: a b c \0 -- break
		// cases: a b c \n ... \n \0 -- trim delim in the ends as follows then break
		if (!buff_size) break;

		// find start of next line
		line_len = 1; // borrows variable line_len, exactly means delim length
		line_start = line_end + 1;
		while(*line_start == '\n') {
			line_start++, line_len++;
		}
		buff_size -= line_len;
	}

	if(line_pack_size != 0) {
		line_pack->size = line_pack_size;
		line_ptr_rb.enqueue();

		line_pack = line_ptr_rb.enqueue_acquire();
		line_pack_size = 0;
	}

	line_pack->size = line_pack_size = 0;
	line_pack->lines[0] = string_view(nullptr, 0);
	line_ptr_rb.enqueue();
}

void FastqReader::init(){
	if (ends_with(mFilename, ".gz")){
		mZipFileName = (char*)mFilename.c_str();
		mZipped = true;
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
}

// void FastqReader::getBytes(size_t& bytesRead, size_t& bytesTotal) {
// 	if(mZipped) {
// 		bytesRead = gzoffset(mZipFile);
// 	} else {
// 		bytesRead = ftell(mFile);//mFile.tellg();
// 	}

// 	// use another ifstream to not affect current reader
// 	ifstream is(mFilename);
// 	is.seekg (0, is.end);
// 	bytesTotal = is.tellg();
// }

// void FastqReader::clearLineBreaks(char* line) {

// 	// trim \n, \r or \r\n in the tail
// 	int readed = strlen(line);
// 	if(readed >=2 ){
// 		if(line[readed-1] == '\n' || line[readed-1] == '\r'){
// 			line[readed-1] = '\0';
// 			if(line[readed-2] == '\r')
// 				line[readed-2] = '\0';
// 		}
// 	}
// }

const string_view& FastqReader::getLine(){
	LinePack *line_pack = line_pack_outputing;
	size_t n = line_pack_n_outputed;

	if(!line_pack || n >= line_pack->size) {
		if(line_pack) line_ptr_rb.dequeue();

		line_pack_outputing = line_pack = line_ptr_rb.dequeue_acquire();
		line_pack_n_outputed = n = 0;

		if(line_pack->size == 0) {
			mNoLineLeftInRingBuf = true;
			return line_pack->lines[0];
		}
	}

	const string_view& line = line_pack->lines[n];
	line_pack_n_outputed = n + 1;
	return line;
}

// bool FastqReader::eof() {
// 	if (mZipped) {
// 		return gzeof(mZipFile);
// 	} else {
// 		return feof(mFile);//mFile.eof();
// 	}
// }

Read* FastqReader::read(){
	assert(0);
	if (mZipped){
		if (mZipFileName == NULL)
			return NULL;
	}

	if (mNoLineLeftInRingBuf)
		return NULL;

	string_view name = getLine();
	// name should start with @
	while((name.empty() && !mNoLineLeftInRingBuf) || (!name.empty() && name[0]!='@')){
		name = getLine();
	}

	if(name.empty())
		return NULL;

	string_view sequence = getLine();
	string_view strand = getLine();

	// WAR for FQ with no quality
	// if (!mHasQuality){
	// 	string quality = string(sequence.length(), 'K');
	// 	return new Read(name, sequence, strand, quality, mPhred64);
	// }
	// else {
	assert(mHasQuality);
	string_view quality = getLine();
	if(quality.length() != sequence.length()) {
		cerr << "ERROR: sequence and quality have different length:" << endl;
		cerr << name << endl;
		cerr << sequence << endl;
		cerr << strand << endl;
		cerr << quality << endl;
		return NULL;
	}
	return new Read(name, sequence, strand, quality, mPhred64);
	// }
}

Read* FastqReader::read(Read* dst){
	if (mZipped){
		if (mZipFileName == NULL)
			return NULL;
	}

	if (mNoLineLeftInRingBuf)
		return NULL;

	string_view name = getLine();
	// name should start with @
	while((name.empty() && !mNoLineLeftInRingBuf) || (!name.empty() && name[0]!='@')){
		name = getLine();
	}

	if(name.empty())
		return NULL;

	string_view sequence = getLine();
	string_view strand = getLine();

	// WAR for FQ with no quality
	// if (!mHasQuality){
	// 	string quality = string(sequence.length(), 'K');
	// 	return new Read(name, sequence, strand, quality, mPhred64);
	// }
	// else {
	assert(mHasQuality);
	string_view quality = getLine();
	if(quality.length() != sequence.length()) {
		cerr << "ERROR: sequence and quality have different length:" << endl;
		cerr << name << endl;
		cerr << sequence << endl;
		cerr << strand << endl;
		cerr << quality << endl;
		return NULL;
	}
	dst->mName = name;
	dst->mSeq = sequence;
	dst->mStrand = strand;
	dst->mQuality = quality;

	return dst;
}

void FastqReader::close(){
	if (mZipped){
		if (mZipFileName){
			mZipFileName = NULL;
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

// bool FastqReader::test(){
// 	FastqReader reader1("testdata/R1.fq");
// 	FastqReader reader2("testdata/R1.fq.gz");
// 	Read* r1 = NULL;
// 	Read* r2 = NULL;
// 	while(true){
// 		r1=reader1.read();
// 		r2=reader2.read();
// 		if(r1 == NULL || r2 == NULL)
// 			break;
// 		if(r1->mSeq.mStr != r2->mSeq.mStr){
// 			return false;
// 		}
// 		delete r1;
// 		delete r2;
// 	}
// 	return true;
// }

FastqReaderPair::FastqReaderPair(FastqReader* left, FastqReader* right){
	mLeft = left;
	mRight = right;
}

FastqReaderPair::FastqReaderPair(string leftName, string rightName, bool hasQuality, bool phred64, bool interleaved){
	assert(false);
	mInterleaved = interleaved;
	mLeft = new FastqReader(leftName, NULL, hasQuality, phred64);
	if(mInterleaved)
		mRight = NULL;
	else
		mRight = new FastqReader(rightName, NULL, hasQuality, phred64);
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

