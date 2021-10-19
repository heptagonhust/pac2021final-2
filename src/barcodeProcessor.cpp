#include "barcodeProcessor.h"
#include <string_view>

BarcodeProcessor::BarcodeProcessor(Options* opt, BarcodeMap* mbpmap)
{
	// string_view OK
	mOptions = opt;
	bpmap = mbpmap;
	mismatch = opt->transBarcodeToPos.mismatch;
	barcodeLen = opt->barcodeLen;
	polyTInt = seqEncode(string_view{polyT}, 0, barcodeLen, mOptions->rc);
	misMaskGenerate();
}

BarcodeProcessor::BarcodeProcessor()
{
}

BarcodeProcessor::~BarcodeProcessor()
{

}

bool BarcodeProcessor::process(Read* read1, Read* read2)
{
	// string_view OK
	totalReads++;
	string_view barcode;
	string_view barcodeQ;
	if (mOptions->transBarcodeToPos.barcodeRead == 1){
		barcode = read1->mSeq.mStr.substr(mOptions->barcodeStart, mOptions->barcodeLen);
		barcodeQ = read1->mQuality.substr(mOptions->barcodeStart, mOptions->barcodeLen);
	}else if (mOptions->transBarcodeToPos.barcodeRead == 2){
		barcode = read2->mSeq.mStr.substr(mOptions->barcodeStart, mOptions->barcodeLen);
		barcodeQ = read2->mQuality.substr(mOptions->barcodeStart, mOptions->barcodeLen);
	}else{
		error_exit("barcodeRead must be 1 or 2 . please check the --barcodeRead option you give");
	}
	barcodeStatAndFilter(barcodeQ);
	Position1* position = getPosition(barcode);

	if (position != nullptr){
		mMapToSlideRead++ ;
		bool umiPassFilter = true;
		if (mOptions->transBarcodeToPos.umiStart >= 0 && mOptions->transBarcodeToPos.umiLen > 0){
			pair<string_view, string_view> umi;
			if (mOptions->transBarcodeToPos.umiRead == 1){
				getUMI(read1, umi);
			}else{
				getUMI(read2, umi, true);
			}
			umiPassFilter = umiStatAndFilter(umi);
			if (!mOptions->transBarcodeToPos.PEout){
				addPositionToName(read2, position, &umi);
			}
			assert(!mOptions->transBarcodeToPos.PEout);
			// Comment this function to support type `string` transformation.
			// else{
			// 	addPositionToNames(read1, read2, position, &umi);
			// }
		}
		else{
			assert(false);
			// Comment this function to support type `string` transformation.
			// if (!mOptions->transBarcodeToPos.PEout){
			// 	addPositionToName(read2, position);
			// }
			// assert(!mOptions->transBarcodeToPos.PEout);

			// Comment this function to support type `string` transformation.
			// else{
			// 	addPositionToNames(read1, read2, position);
			// }
		}
		// Comment this function to support type `string` transformation.
		// if(! mOptions->transBarcodeToPos.mappedDNBOutFile.empty())
		// 	addDNB(encodePosition(position->x, position->y));

		return umiPassFilter;
	}
	return false;

}

void BarcodeProcessor::addPositionToName(Read* r, Position1* position, pair<string_view, string_view>* umi)
{
	// string_view OK
	string position_tag = positionToString(position);
	int readTagPos = r->mName.find("/");

	stringstream str;
	if (readTagPos!=string_view::npos){
		str << r->mName.substr(0, readTagPos);
	}else{
		str << r->mName;
	}
	if (umi == NULL)
		assert(false);
		// r->mName = readName + "|||CB:Z:" + position_tag;
	else {
		str << "|||CB:Z:" << position_tag << "|||UR:Z:" << umi->first << "|||UY:Z:" << umi->second;
		string *nonVolatileBuf = new string{};
		str >> *nonVolatileBuf;
		r->mName = string_view{*nonVolatileBuf};
	}
}

// void BarcodeProcessor::addPositionToNames(Read* r1, Read* r2, Position1* position, pair<string, string>* umi){
// 	string position_tag = positionToString(position);
// 	int readTagPos = r1->mName.find("/");
// 	string readName;
// 	if (readTagPos!=string::npos){
// 		readName = r1->mName.substr(0, readTagPos);
// 	}else{
// 		readName = r1->mName;
// 	}
// 	if (umi == NULL){
// 		r1->mName = readName + "|||CB:Z:" + position_tag;
// 		if (mOptions->transBarcodeToPos.barcodeRead == 1){
// 			r1->trimBack(mOptions->barcodeStart);
// 		}else{
// 			r2->trimBack(mOptions->barcodeStart);
// 		}
// 		r2->mName = r1->mName;
// 	}else{
// 		r1->mName = readName + "|||CB:Z:" + position_tag + "|||UR:Z:" + umi->first+ "|||UY:Z:" + umi->second;
// 		if (mOptions->transBarcodeToPos.umiRead == 1 && mOptions->transBarcodeToPos.barcodeRead == 1){
// 			int trimStart = mOptions->barcodeStart > mOptions->transBarcodeToPos.umiStart ? mOptions->transBarcodeToPos.umiStart : mOptions->barcodeStart;
// 			r1->trimBack(trimStart);
// 		}else{
// 			r2->trimBack(mOptions->barcodeStart);
// 		}
// 		r2->mName = r1->mName;
// 	}
// }

void BarcodeProcessor::getUMI(Read* r, pair<string_view, string_view>& umi, bool isRead2)
{
	// string_view OK
	string_view umiSeq = r->mSeq.mStr.substr(mOptions->transBarcodeToPos.umiStart, mOptions->transBarcodeToPos.umiLen);
	string_view umiQ = r->mQuality.substr(mOptions->transBarcodeToPos.umiStart, mOptions->transBarcodeToPos.umiLen);
	umi.first = umiSeq;
	umi.second = umiQ;
	if (isRead2) {
		r->mSeq.mStr = r->mSeq.mStr.substr(0, mOptions->transBarcodeToPos.umiStart);
		r->mQuality = r->mQuality.substr(0, mOptions->transBarcodeToPos.umiStart);
	}
}


void BarcodeProcessor::decodePosition(const uint32 codePos, pair<uint16, uint16>& decodePos)
{
	decodePos.first = codePos >> 16;
	decodePos.second = codePos & 0x0000FFFF;
}

void BarcodeProcessor::decodePosition(const uint64 codePos, pair<uint32, uint32>& decodePos)
{
	decodePos.first = codePos >> 32;
	decodePos.second = codePos & 0x00000000FFFFFFFF;
}

uint32 BarcodeProcessor::encodePosition(int fovCol, int fovRow)
{
	uint32 encodePos = (fovCol << 16) | fovRow;
	return encodePos;
}

uint64 BarcodeProcessor::encodePosition(uint32 x, uint32 y)
{
	uint64 encodePos = ((uint64)x << 32) | (uint64)y;
	return encodePos;
}

long BarcodeProcessor::getBarcodeTypes()
{
	return bpmap->size();
}

Position1* BarcodeProcessor::getPosition(uint64 barcodeInt)
{
	// string_view OK
	MapIter iter = bpmap->find(barcodeInt);
	if (iter!=bpmap->end()) {
		overlapReads++;
		return &iter->second;
	}
	else if (mismatch > 0) {
		iter = getMisOverlap(barcodeInt);
		if (iter != bpmap->end()) {
			overlapReadsWithMis++;
			return &iter->second;
		}
		else {
			return nullptr;
		}
	}
	return nullptr;
}

Position1* BarcodeProcessor::getPosition(string_view& barcodeString)
{
	// string_view OK
	int Nindex = getNindex(barcodeString);
	if (Nindex == -1) {
		uint64 barcodeInt = seqEncode(barcodeString, 0, barcodeLen);
		if (barcodeInt == polyTInt) {
			return nullptr;
		}
		return getPosition(barcodeInt);
	}
	else if (Nindex == -2) {
		return nullptr;
	}
	else if (mismatch > 0) {
		return getNOverlap(barcodeString, Nindex);
	}
	return nullptr;
}

void BarcodeProcessor::misMaskGenerate()
{
	// string_view OK
	misMaskLen = possibleMis(barcodeLen, mismatch);
	misMaskLens = new int[mismatch];
	for (int i=0; i< mismatch; i++){
		misMaskLens[i] = possibleMis(barcodeLen, i+1);
	}
	
	misMask = (uint64*)malloc(misMaskLen * sizeof(uint64));
	set<uint64> misMaskSet;
	int index = 0;
	if (mismatch > 0) {
		for (int i = 0; i < barcodeLen; i++) {
			for (uint64 j = 1; j < 4; j++) {
				uint64 misMaskInt = j << i * 2;
				misMask[index] = misMaskInt;
				index++;
			}
		}
		if (mOptions->verbose) {
			string msg = "1 mismatch mask barcode number: " + to_string(index);
			loginfo(msg);
		}
	}
	if (mismatch == 2) {
		misMaskSet.clear();
		for (int i = 0; i < barcodeLen; i++) {
			for (uint64 j = 1; j < 4; j++) {
				uint64 misMaskInt1 = j << i * 2;
				for (int k = 0; k < barcodeLen; k++) {
					if (k == i) {
						continue;
					}
					for (uint64 j2 = 1; j2 < 4; j2++) {
						uint64 misMaskInt2 = j2 << k * 2;
						uint64 misMaskInt = misMaskInt1 | misMaskInt2;
						misMaskSet.insert(misMaskInt);
					}
				}
			}
		}
		for (auto iter = misMaskSet.begin(); iter != misMaskSet.end(); iter++) {
			misMask[index] = *iter;
			index++;
		}
		if (mOptions->verbose) {
			string msg = "2 mismatch mask barcode number: " + to_string(misMaskSet.size());
			loginfo(msg);
		}
	}
	if (mismatch == 3) {
		misMaskSet.clear();
		for (int i = 0; i < barcodeLen; i++) {
			for (uint64 j = 1; j < 4; j++) {
				uint64 misMaskInt1 = j << i * 2;
				for (int k = 0; k < barcodeLen; k++) {
					if (k == i) {
						continue;
					}
					for (uint64 j2 = 1; j2 < 4; j2++) {
						uint64 misMaskInt2 = j2 << k * 2;
						for (int h = 0; h < barcodeLen; h++) {
							if (h == k || h == i) {
								continue;
							}
							for (uint64 j3 = 1; j3 < 4; j3++) {
								uint64 misMaskInt3 = j3 << h * 2;
								uint64 misMaskInt = misMaskInt1 | misMaskInt2 | misMaskInt3;
								misMaskSet.insert(misMaskInt);
							}
						}
					}
				}
			}
		}
		for (auto iter = misMaskSet.begin(); iter != misMaskSet.end(); iter++) {
			misMask[index] = *iter;
			index++;
		}
		if (mOptions->verbose) {
			string msg = "3 mismatch mask barcode number: " + to_string(misMaskSet.size());
			loginfo(msg);
		}
		misMaskSet.clear();
	}
	if (mOptions->verbose) {
		string msg = "total mismatch mask length: " + to_string(misMaskLen);
		loginfo(msg);
	}
}

string BarcodeProcessor::positionToString(Position* position)
{
	stringstream positionString;
	positionString << position->x << "_" << position->y;
	return positionString.str();
}

string BarcodeProcessor::positionToString(Position1* position){
	stringstream positionString;
	positionString << position->x << "_" << position->y;
	return positionString.str();
}

MapIter BarcodeProcessor::getMisOverlap(uint64 barcodeInt)
{
	// string_view OK
	uint64 misBarcodeInt;
	int misCount = 0;
	int misMaskIndex = 0;
	MapIter iter;
	MapIter overlapIter;

	for (int mis = 0; mis < mismatch; mis++){
		misCount = 0;
		while (misMaskIndex < misMaskLens[mis]) {
			misBarcodeInt = barcodeInt ^ misMask[misMaskIndex];
			misMaskIndex++;
			iter = bpmap->find(misBarcodeInt);
			if (iter != bpmap->end()) {
				overlapIter = iter;
				misCount++;
				if (misCount > 1) {
					return bpmap->end();
				}
			}
		}
		if (misCount == 1) {
			return overlapIter;
		}
	}
	return bpmap->end();
}

Position1* BarcodeProcessor::getNOverlap(string_view& barcodeString, uint8 Nindex)
{
	// string_view OK
	//N has the same encode (11) with G
	int misCount = 0;
	uint64 barcodeInt = seqEncode(barcodeString, 0, barcodeString.length());
	BarcodeMap::iterator iter;
	BarcodeMap::iterator overlapIter;
	iter = bpmap->find(barcodeInt);
	if (iter!=bpmap->end()) {
		misCount++;
		overlapIter = iter;
	}
	for (uint64 j = 1; j < 4; j++) {
		uint64 misBarcodeInt = barcodeInt ^ (j << Nindex * 2);
		iter = bpmap->find(misBarcodeInt);
		if (iter != bpmap->end()) {
			misCount++;
			if (misCount > 1) {
				return nullptr;
			}
			overlapIter = iter;
		}
	}
	if (misCount == 1) {
		overlapReadsWithN++;
		return &overlapIter->second;
	}
	return nullptr;
}

int BarcodeProcessor::getNindex(string_view& barcodeString)
{
	// string_view OK
	int Nindex = barcodeString.find("N");
	if (Nindex == barcodeString.npos) {
		return -1;
	}
	else if (Nindex != barcodeString.rfind("N")) {
		return -2;
	}
	return Nindex;
}

// void BarcodeProcessor::addDNB(uint64 barcodeInt)
// {
// 	if (mDNB.count(barcodeInt) > 0) {
// 		mDNB[barcodeInt]++;
// 	}
// 	else {
// 		mDNB[barcodeInt]++;
// 	}
// }

// bool BarcodeProcessor::barcodeStatAndFilter(pair<string, string>& barcode)
// {
// 	for (int i = 0; i < barcodeLen; i++) {
// 		if (barcode.second[i] >= q30) {
// 			barcodeQ30++;
// 			barcodeQ20++;
// 			barcodeQ10++;
// 		}
// 		else if (barcode.second[i] >= q20) {
// 			barcodeQ20++;
// 			barcodeQ10++;
// 		}else if (barcode.second[i] >= q10) {
// 			barcodeQ10++;
// 		}
// 	}
// 	return true;
// }

bool BarcodeProcessor::barcodeStatAndFilter(string_view& barcodeQ)
{
	// string_view OK
	for (int i = 0; i < barcodeLen; i++) {
		if (barcodeQ[i] >= q30) {
			barcodeQ30++;
			barcodeQ20++;
			barcodeQ10++;
		}
		else if (barcodeQ[i] >= q20) {
			barcodeQ20++;
			barcodeQ10++;
		}else if (barcodeQ[i] >= q10) {
			barcodeQ10++;
		}
	}
	return true;
}

bool BarcodeProcessor::umiStatAndFilter(pair<string_view, string_view>& umi)
{
	// string_view OK
	int q10BaseCount = 0;
	for (int i = 0; i < mOptions->transBarcodeToPos.umiLen; i++) {
		if (umi.second[i] >= q30) {
			umiQ30++;
			umiQ20++;
			umiQ10++;
		}
		else if (umi.second[i] >= q20) {
			umiQ20++;
			umiQ10++;
		}else if (umi.second[i] >= q10) {
			umiQ10++;
		}else{
			q10BaseCount++;
		}
	}
	if (umi.first.find("N") != string_view::npos){
		umiNFilterReads++;
		return false;
	}else if (seqEncode(umi.first, 0, mOptions->transBarcodeToPos.umiLen) == 0){
		umiPloyAFilterReads++;
		return false;
	}else if (q10BaseCount>1){
		umiQ10FilterReads++;
		return false;
	}else{
		return true;
	}
}

void BarcodeProcessor::dumpDNBmap(string& dnbMapFile){
	ofstream writer;
	if (ends_with(dnbMapFile, ".bin")) {
		mDNB.reserve(mDNB.size());
		writer.open(dnbMapFile, ios::out | ios::binary);
		boost::archive::binary_oarchive oa(writer);
		oa << mDNB;
	}
	else {
		writer.open(dnbMapFile);
		unordered_map<uint64, int>::iterator mapIter = mDNB.begin();
		while (mapIter != mDNB.end()) {
			uint32 x = mapIter->first >> 32;
			uint32 y = mapIter->first & 0x00000000FFFFFFFF;
			writer << x << "\t" << y << "\t" << mapIter->second << endl;
			mapIter++;
		}
	}
	writer.close();
}
