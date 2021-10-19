#include "writerThread.h"

WriterThread::WriterThread(string filename, int compressionLevel)
	: rb(512 * 1024 * 16)
{
	compression = compressionLevel;

	mWriter1 = NULL;

	mInputCompleted = false;
	mFilename = filename;

	initWriter(filename);
}

WriterThread::~WriterThread()
{
	cleanup();
}

bool WriterThread::setInputCompleted() {
	OutputStrPack *pack = rb.enqueue_acquire();
	pack->size = 0;
		
	rb.enqueue();
	return true;
}

void WriterThread::outputTask() {
	while(true) {
		OutputStrPack *pack = rb.dequeue_acquire();
		if(pack->size == 0)
			break;
		
		mWriter1->write(pack->data, pack->size);
		rb.dequeue();
	}
}

void WriterThread::input(const char* data, size_t size) {
	size_t n_outputed = 0;
	while(n_outputed < size) {
		int n = size - n_outputed;
		n = n < WRITER_OUTPUT_PACK_SIZE ? n : WRITER_OUTPUT_PACK_SIZE;

		OutputStrPack *pack = rb.enqueue_acquire();

		pack->size = n;
		memcpy(pack->data, &data[n_outputed], n);

		rb.enqueue();
		n_outputed += n;
	}
}

void WriterThread::cleanup() {
	deleteWriter();
}

void WriterThread :: deleteWriter() {
	if (mWriter1 != NULL) {
		delete mWriter1;
		mWriter1 = NULL;
	}
}

void WriterThread::initWriter(string filename1) {
	deleteWriter();
	mWriter1 = new Writer(filename1, compression);
}

void WriterThread::initWriter(ofstream* stream) {
	deleteWriter();
	mWriter1 = new Writer(stream);
}

// void WriterThread::initWriter(gzFile gzfile) {
// 	deleteWriter();
// 	mWriter1 = new Writer(gzfile);
// }

long WriterThread::bufferLength() {
	assert(0);
	return 0;
}
