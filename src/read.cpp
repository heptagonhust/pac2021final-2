#include "read.h"
#include <sstream>
#include <string_view>
#include "util.h"

Read::Read(string_view name, string_view seq, string_view strand, string_view quality, bool phred64){
	mName = name;
	mSeq = Sequence(seq);
	mStrand = strand;
	mQuality = quality;
	mHasQuality = true;
	assert(!phred64);
	// Since phred64 is always false, comment the function to support type `string` transformation.
	// if(phred64)
	// 	convertPhred64To33();
}

// Read::Read(string name, string seq, string strand){
// 	mName = name;
// 	mSeq = Sequence(seq);
// 	mStrand = strand;
// 	mHasQuality = false;
// }

// Read::Read(string name, Sequence seq, string strand, string quality, bool phred64){
// 	mName = name;
// 	mSeq = seq;
// 	mStrand = strand;
// 	mQuality = quality;
// 	mHasQuality = true;
// 	if(phred64)
// 		convertPhred64To33();
// }

// Read::Read(string name, Sequence seq, string strand){
// 	mName = name;
// 	mSeq = seq;
// 	mStrand = strand;
// 	mHasQuality = false;
// }

// void Read::convertPhred64To33(){
// 	for(int i=0; i<mQuality.length(); i++) {
// 		mQuality[i] = max(33, mQuality[i] - (64-33));
// 	}
// }

Read::Read(Read &r) {
	mName = r.mName;
	mSeq = r.mSeq;
	mStrand = r.mStrand;
	mQuality = r.mQuality;
	mHasQuality = r.mHasQuality;
}

Read::Read()
{
}

Read::~Read()
{
}

void Read::print(){
	std::cerr << mName << endl;
	std::cerr << mSeq.mStr << endl;
	std::cerr << mStrand << endl;
	if(mHasQuality)
		std::cerr << mQuality << endl;
}

// void Read::printFile(ofstream& file){
// 	file << mName << endl;
// 	file << mSeq.mStr << endl;
// 	file << mStrand << endl;
// 	if(mHasQuality)
// 		file << mQuality << endl;
// }

// Since FastMerge is not used, comment the function to support type `string` transformation.
// Read* Read::reverseComplement(){
// 	Sequence seq = ~mSeq;
// 	string qual;
// 	qual.assign(mQuality.rbegin(), mQuality.rend());
// 	string strand = (mStrand=="+") ? "-" : "+";
// 	return new Read(mName, seq, strand, qual);
// }

// void Read::resize(int len) {
// 	if(len > length() || len<0)
// 		return ;
// 	mSeq.mStr.resize(len);
// 	mQuality.resize(len);
// }
   
// void Read::trimFront(int len){
// 	if (len > length()-1){
// 		mSeq.mStr = "";
// 		mQuality = "";
// 	}else{
// 		mSeq.mStr = mSeq.mStr.substr(len, mSeq.mStr.length() - len);
// 		mQuality = mQuality.substr(len, mQuality.length() - len);
// 	}
// }

void Read::trimBack(int start){
	if (start >= length()){
		return;
	}else{
		mSeq.mStr = mSeq.mStr.substr(0, start);
		mQuality = mQuality.substr(0, start);
	}
}

// void Read::getDNBidx(bool isSeq500, int suffixLen, int indexLen)
// {
// 	//dnb_index, fov_col, fov_row
// 	int nameLength = mName.size()-suffixLen;
// 	if (isSeq500) {
// 		size_t pos = mName.find("_");
// 		string temp = mName.substr(0, pos);
// 		dnbIdx[0] = stoi(mName.substr(pos + 1, nameLength));
// 		dnbIdx[2] = stoi(temp.substr(temp.size() - 3, 3));
// 		dnbIdx[1] = stoi(temp.substr(temp.size() - 7, 3));
// 	}
// 	else {
// 		dnbIdx[0] = stoi(mName.substr(nameLength -indexLen, indexLen));
// 		dnbIdx[2] = stoi(mName.substr(nameLength -indexLen-3, 3));
// 		dnbIdx[1] = stoi(mName.substr(nameLength -indexLen-7, 3));
// 	}
// }

// void Read::getBarcodeFromName(int barcodeLen)
// {
// 	mBarcode = mName.substr(1, barcodeLen);
// 	//cout << mBarcode << endl;
// 	int sepIndex = mBarcode.find("@");
// 	if (sepIndex != mBarcode.npos) {
// 		cerr << "The barcode length of read:  " << mName << "  is: " << sepIndex << " but you set the barcode length: " << barcodeLen << endl;
// 	}
// }

// string Read::lastIndex(){
// 	int len = mName.length();
// 	if(len<5)
// 		return "";
// 	for(int i=len-3;i>=0;i--){
// 		if(mName[i]==':' || mName[i]=='+'){
// 			return mName.substr(i+1, len-i);
// 		}
// 	}
// 	return "";
// }

// string Read::firstIndex(){
// 	int len = mName.length();
// 	int end = len;
// 	if(len<5)
// 		return "";
// 	for(int i=len-3;i>=0;i--){
// 		if(mName[i]=='+')
// 			end = i-1;
// 		if(mName[i]==':'){
// 			return mName.substr(i+1, end-i);
// 		}
// 	}
// 	return "";
// }

// int Read::lowQualCount(int qual){
// 	int count = 0;
// 	for(int q=0;q<mQuality.size();q++){
// 		if(mQuality[q] < qual + 33)
// 			count++;
// 	}
// 	return count;
// }

int Read::length(){
	return mSeq.length();
}

inline void string_view_append(char *dst, string_view* src) {
	for (int i = 0; i < src->length(); ++i) {
		*dst = (*src)[i];
		dst ++;
	}
	*dst = '\n';
	dst ++;
	*dst = '\0';
}

char* Read::toNewCharPointer() {
	// mName + "\n" + mSeq.mStr + "\n" + mStrand + "\n" + mQuality + "\n";
	size_t nameLen = mName.length(), seqLen = mSeq.mStr.length(),
				strandLen = mStrand.length(), qualityLen = mQuality.length();
	char *str = new char[nameLen + seqLen + strandLen + qualityLen + 5];
	char *ret = str;
	string_view_append(str, &mName);
	str += nameLen + 1;
	string_view_append(str, &mSeq.mStr);
	str += seqLen + 1;
	string_view_append(str, &mStrand);
	str += strandLen + 1;
	string_view_append(str, &mQuality);
	return ret;
}

// string Read::toStringWithTag(string tag) {
// 	return mName + " " + tag + "\n" + mSeq.mStr + "\n" + mStrand + "\n" + mQuality + "\n";
// }

// bool Read::test(){
// 	Read r("@NS500713:64:HFKJJBGXY:1:11101:20469:1097 1:N:0:TATAGCCT+GGTCCCGA",
// 		"CTCTTGGACTCTAACACTGTTTTTTCTTATGAAAACACAGGAGTGATGACTAGTTGAGTGCATTCTTATGAGACTCATAGTCATTCTATGATGTAGTTTTCCTTAGGAGGACATTTTTTACATGAAATTATTAACCTAAATAGAGTTGATC",
// 		"+",
// 		"AAAAA6EEEEEEEEEEEEEEEEE#EEEEEEEEEEEEEEEEE/EEEEEEEEEEEEEEEEAEEEAEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE<EEEEAEEEEEEEEEEEEEEEAEEE/EEEEEEEEEEAAEAEAAEEEAEEAA");
// 	string idx = r.lastIndex();
// 	return idx == "GGTCCCGA";
// }

ReadPair::ReadPair(Read* left, Read* right){
	mLeft = left;
	mRight = right;
}

ReadPair::ReadPair(){
	
}

ReadPair::~ReadPair(){
	if(mLeft){
		delete mLeft;
		mLeft = NULL;
	}
	if(mRight){
		delete mRight;
		mRight = NULL;
	}
}

// Read* ReadPair::fastMerge(){
// 	Read* rcRight = mRight->reverseComplement();
// 	int len1 = mLeft->length();
// 	int len2 = rcRight->length();
// 	// use the pointer directly for speed
// 	const char* str1 = mLeft->mSeq.mStr.c_str();
// 	const char* str2 = rcRight->mSeq.mStr.c_str();
// 	const char* qual1 = mLeft->mQuality.c_str();
// 	const char* qual2 = rcRight->mQuality.c_str();

// 	// we require at least 30 bp overlapping to merge a pair
// 	const int MIN_OVERLAP = 30;
// 	bool overlapped = false;
// 	int olen = MIN_OVERLAP;
// 	int diff = 0;
// 	// the diff count for 1 high qual + 1 low qual
// 	int lowQualDiff = 0;

// 	while(olen <= min(len1, len2)){
// 		diff = 0;
// 		lowQualDiff = 0;
// 		bool ok = true;
// 		int offset = len1 - olen;
// 		for(int i=0;i<olen;i++){
// 			if(str1[offset+i] != str2[i]){
// 				diff++;
// 				// one is >= Q30 and the other is <= Q15
// 				if((qual1[offset+i]>='?' && qual2[i]<='0') || (qual1[offset+i]<='0' && qual2[i]>='?')){
// 					lowQualDiff++;
// 				}
// 				// we disallow high quality diff, and only allow up to 3 low qual diff
// 				if(diff>lowQualDiff || lowQualDiff>=3){
// 					ok = false;
// 					break;
// 				}
// 			}
// 		}
// 		if(ok){
// 			overlapped = true;
// 			break;
// 		}
// 		olen++;
// 	}

// 	if(overlapped){
// 		int offset = len1 - olen;
// 		stringstream ss;
// 		ss << mLeft->mName << " merged offset:" << offset << " overlap:" << olen << " diff:" << diff;
// 		string mergedName = ss.str();
// 		string mergedSeq = mLeft->mSeq.mStr.substr(0, offset) + rcRight->mSeq.mStr;
// 		string mergedQual = mLeft->mQuality.substr(0, offset) + rcRight->mQuality;
// 		// quality adjuction and correction for low qual diff
// 		for(int i=0;i<olen;i++){
// 			if(str1[offset+i] != str2[i]){
// 				if(qual1[offset+i]>='?' && qual2[i]<='0'){
// 					mergedSeq[offset+i] = str1[offset+i];
// 					mergedQual[offset+i] = qual1[offset+i];
// 				} else {
// 					mergedSeq[offset+i] = str2[i];
// 					mergedQual[offset+i] = qual2[i];
// 				}
// 			} else {
// 				// add the quality of the pair to make a high qual
// 				mergedQual[offset+i] =  qual1[offset+i] + qual2[i] - 33;
// 			}
// 		}
// 		delete rcRight;
// 		return new Read(mergedName, mergedSeq, "+", mergedQual);
// 	}

// 	delete rcRight;
// 	return NULL;
// }

// bool ReadPair::test(){
// 	Read* left = new Read("@NS500713:64:HFKJJBGXY:1:11101:20469:1097 1:N:0:TATAGCCT+GGTCCCGA",
// 		"TTTTTTCTCTTGGACTCTAACACTGTTTTTTCTTATGAAAACACAGGAGTGATGACTAGTTGAGTGCATTCTTATGAGACTCATAGTCATTCTATGATGTAG",
// 		"+",
// 		"AAAAA6EEEEEEEEEEEEEEEEE#EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEAEEEAEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
// 	Read* right = new Read("@NS500713:64:HFKJJBGXY:1:11101:20469:1097 1:N:0:TATAGCCT+GGTCCCGA",
// 		"AAAAAACTACACCATAGAATGACTATGAGTCTCATAAGAATGCACTCAACTAGTCATCACTCCTGTGTTTTCATAAGAAAAAACAGTGTTAGAGTCCAAGAG",
// 		"+",
// 		"AAAAA6EEEEE/EEEEEEEEEEE#EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEAEEEAEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");

// 	ReadPair pair(left, right);
// 	Read* merged = pair.fastMerge();
// 	if(merged == NULL)
// 		return false;

// 	if(merged->mSeq.mStr != "TTTTTTCTCTTGGACTCTAACACTGTTTTTTCTTATGAAAACACAGGAGTGATGACTAGTTGAGTGCATTCTTATGAGACTCATAGTCATTCTATGATGTAGTTTTTT")
// 		return false;

// 	return true;
// }
