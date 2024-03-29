#ifndef  BARCODE_PROCESSOR_H
#define BARCODE_PROCESSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include "read.h"
#include "util.h"
#include "barcodePositionMap.h"
#include "options.h"
#include "util.h"

using namespace std;

class BarcodeProcessor {
public:
	BarcodeProcessor(Options* opt, BarcodeMap* mbpmap);
	BarcodeProcessor();
	~BarcodeProcessor();
	bool process(Read* read1, Read* read2);
	bool processToStrOnly(Read* read1, Read* read2,  ostream * mapped_out, ostream *unmapped_out);
	void dumpDNBmap(string& dnbMapFile);
private:
	void addPositionToName(Read* r, Position1* position, pair<string_view, string_view>* umi = NULL);
	void outputReadWithPosition(Read* r, Position1* position, pair<string_view, string_view>* umi, ostream *dst);
	void outputRead(Read* r, Position1* position, pair<string_view, string_view>* umi, ostream *dst);
	// void addPositionToNames(Read* r1, Read* r2, Position1* position, pair<string, string>* umi = NULL);
	void  getUMI(Read* r, pair<string_view, string_view>& umi, bool isRead2=false);
	void decodePosition(const uint32 codePos, pair<uint16, uint16>& decodePos);
	void decodePosition(const uint64 codePos, pair<uint32, uint32>& decodePos);
	uint32 encodePosition(int fovCol, int fovRow);
	uint64 encodePosition(uint32 x, uint32 y);
	long getBarcodeTypes();
	Position1* getPosition(uint64 barcodeInt);
	Position1* getPosition(string_view& barcodeString);
	void misMaskGenerate();
	string positionToString(Position1* position);
	string positionToString(Position* position);
	BarcodeMap::iterator getMisOverlap(uint64 barcodeInt);
	Position1* getNOverlap(string_view& barcodeString, uint8 Nindex);
	int getNindex(string_view& barcodeString);
	// void addDNB(uint64 barcodeInt);
	// bool barcodeStatAndFilter(pair<string, string>& barcode);
	bool barcodeStatAndFilter(string_view& barcodeQ);
	bool umiStatAndFilter (pair<string_view, string_view>& umi);
private:
	uint64* misMask;
	int misMaskLen;
	int* misMaskLens;
	const char q10 = '+';
	const char q20 = '5';
	const char q30 = '?';
	string polyT = "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTT";
	uint64 polyTInt;
public:
	Options* mOptions;
	BarcodeMap* bpmap;
	long totalReads = 0;
	long mMapToSlideRead = 0;
	long overlapReads = 0;
	long overlapReadsWithMis = 0;
	long overlapReadsWithN = 0;
	long barcodeQ10 = 0;
	long barcodeQ20 = 0;
	long barcodeQ30 = 0;
	long umiQ10 = 0;
	long umiQ20 = 0;
	long umiQ30 = 0;
	long umiQ10FilterReads = 0;
	long umiNFilterReads = 0;
	long umiPloyAFilterReads = 0;
	unordered_map<uint64, int> mDNB;
	int mismatch;
	int barcodeLen;
};


#endif // ! BARCODE_PROCESSOR_H
