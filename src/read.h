#ifndef READ_H
#define READ_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include "sequence.h"
#include <string_view>
#include <vector>

using namespace std;

class Read{
public:
	Read(string_view name, string_view seq, string_view strand, string_view quality, bool phred64=false);
    // Read(string name, Sequence seq, string strand, string quality, bool phred64=false);
	// Read(string name, string seq, string strand);
    // Read(string name, Sequence seq, string strand);
    Read(Read &r);
    Read();
	~Read();
	void print();
    // void printFile(ofstream& file);
    Read* reverseComplement();
    // string firstIndex();
    // string lastIndex();
    // default is Q20
    // int lowQualCount(int qual=20);
    int length();
    // string toString(); // replace it with toNewCharPointer
    char* toNewCharPointer();
    // string toStringWithTag(string tag);
    // void resize(int len);
    // void convertPhred64To33();
    // void trimFront(int len);
    void trimBack(int start);
	// void getDNBidx(bool isSeq500, int suffixLen = 0, int indexLen=8);
    // void getBarcodeFromName(int barcodeLen);

public:
    // static bool test();

private:


public:
	string_view mName;
	Sequence mSeq;
	string_view mStrand;
	string_view mQuality;
	string_view mBarcode;
	// string SEQ500 = "seq500";
	// string DEPSEQT1 = "T1";
	// int dnbIdx[3];
	bool mHasQuality;
};

class ReadPair{
public:
    ReadPair(Read* left, Read* right);
    ReadPair();
    ~ReadPair();

    // merge a pair, without consideration of seq error caused false INDEL
    // Read* fastMerge();
public:
    Read* mLeft;
    Read* mRight;

public:
    // static bool test();
};

#endif