#include "writerThread.h"
#include "barcodeToPositionMulti.h"

#include <iterator>

WriterThread::WriterThread(string filename, int compressionLevel) {
	compression = compressionLevel;

	mWriter1 = NULL;

	mInputCompleted = false;
	mFilename = filename;
	bufferToCompress = NULL;

	initWriter(filename);
}

WriterThread::~WriterThread()
{
	cleanup();
}

inline size_t AppendStringListToChar(StringList* list, char* buffer) {
	size_t merged = 0;
	while (list->next != NULL) {
		memcpy(buffer + merged, list->data, list->data_size);
		merged += list->data_size;
		delete [] list->data;
		list->data = NULL;
		list = list->next;
	}
	return merged;
}

size_t WriterThread::setInputCompleted(int thread_num, char* bufLarge) {
	assert(bufferToCompress == NULL);
	bufferToCompress = bufLarge;
	bufferTotalSize = 0;

	size_t last_append = 0;
	for (int i = 0; i < thread_num; ++ i) {
		last_append = AppendStringListToChar(&headStrList[i], bufLarge);
		bufLarge += last_append;
		bufferTotalSize += last_append;
	}
	*bufLarge = '\0';

	return bufferTotalSize;
}

void WriterThread::outputTask() {
	mWriter1->write(bufferToCompress, bufferTotalSize);
}

void WriterThread::input(int thread_id, const char* data, size_t size) {
	headStrList[thread_id].append((char*)data, size);
	// size_t n_outputed = 0;
	// while(n_outputed < size) {
	// 	int n = size - n_outputed;
	// 	n = n < WRITER_OUTPUT_PACK_SIZE ? n : WRITER_OUTPUT_PACK_SIZE;

	// 	OutputStrPack *pack = rb.enqueue_acquire();

	// 	pack->size = n;
	// 	memcpy(pack->data, &data[n_outputed], n);

	// 	rb.enqueue();
	// 	n_outputed += n;
	// }
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
