#include "fixedfilter.h"
#include <string_view>

FixedFilter::FixedFilter(Options* opt)
{
	mOptions = opt;
	// fixedSequence = opt->transBarcodeToPos.fixedSequence;
	// fixedSequenceRC = reverseComplement(fixedSequence);
	if (opt->transBarcodeToPos.umiRead == 2){
		read1Length = opt->barcodeLen;
	}else if (opt->transBarcodeToPos.umiRead == 1){
		read1Length = 50;
	}
	assert(opt->transBarcodeToPos.fixedSequenceFile.empty());
	// Remove this function to support type `string` transformation.
	// if (!opt->transBarcodeToPos.fixedSequenceFile.empty()) {
	// 	getFxiedSequencesFromFile(opt->transBarcodeToPos.fixedSequenceFile);
	// }	
}

FixedFilter::~FixedFilter()
{
}

bool FixedFilter::filter(Read* read1, Read* read2, Result* result)
{
	// Same to Line 15.
	// if (!mOptions->transBarcodeToPos.fixedSequenceFile.empty()) {
	// 	return filterByMultipleSequences(read1, read2, result);
	// }

	assert(mOptions->transBarcodeToPos.fixedSequence.empty());
	// Remove this function to support type `string` transformation.
	// else if (!mOptions->transBarcodeToPos.fixedSequence.empty()) {
	// 	if (mOptions->transBarcodeToPos.fixedStart < 0) {
	// 		return filterBySequence(read1, read2, result);
	// 	}
	// 	else {
	// 		return filterByPosSpecifySequence(read1, read2, result, mOptions->transBarcodeToPos.fixedStart);
	// 	}
	// }
	return false;
}

// bool FixedFilter::filterBySequence(Read* read1, Read* read2, Result* result)
// {
// 	if (read1->mSeq.mStr.find(fixedSequence) != string::npos || read1->mSeq.mStr.find(fixedSequenceRC) != string::npos) {
// 		result->mFxiedFilterRead++;
// 		return true;
// 	}
// 	return false;
// }

// bool FixedFilter::filterByPosSpecifySequence(Read* read1, Read* read2, Result* result, int posStart)
// {
// 	if (read1->length() < posStart + fixedSequence.length()){
// 		return false;
// 	}
// 	if (read1->mSeq.mStr.find(fixedSequence) == posStart) {
// 		result->mFxiedFilterRead++;
// 		return true;
// 	}
// 	return false;
// }

// bool FixedFilter::filterBySequences(Read* read1, Read* read2, Result* result, string_view fixedSequence){
// 	// string_view OK
// 	if (read1->mSeq.mStr.find(fixedSequence) != string::npos){
// 		result->mFxiedFilterRead++;
// 		return true;
// 	}
// 	return false;
// }

// bool FixedFilter::filterByPosSpecifySequences(Read* read1, Read* read2, Result* result, string_view fixedSequence, int posStart){
// 	// string_view OK
// 	if (read1->length() < posStart + fixedSequence.length()){
// 		return false;
// 	}else if (read1->mSeq.mStr.find(fixedSequence) == posStart){
// 		result->mFxiedFilterRead++;
// 		return true;
// 	}
// 	return false;
// }

// bool FixedFilter::filterByMultipleSequences(Read* read1, Read* read2, Result* result)
// {
// 	// TODO: to string_view
// 	string fixedSequence;
// 	int startPosition;
// 	for (auto fixedIter = fixedSequences.begin(); fixedIter != fixedSequences.end(); fixedIter++) {
// 		fixedSequence = fixedIter->first;
// 		startPosition = fixedIter->second;
// 		if (startPosition >= 0) {
// 			if (filterByPosSpecifySequences(read1, read2, result, fixedSequence, startPosition)) {
// 				return true;
// 			}
// 		}
// 		else {
// 			if (filterBySequences(read1, read2, result, fixedSequence)) {
// 				return true;
// 			}
// 		}
// 	}
// 	return false;
// }

// void FixedFilter::getFxiedSequencesFromFile(string fixedSequenceFile)
// {
// 	ifstream fixedFileStream(fixedSequenceFile);
// 	string fixedSequence;
// 	int startPosition;
// 	while (fixedFileStream >> fixedSequence >> startPosition) {
// 		if (startPosition+fixedSequence.length() <= read1Length){
// 			cerr << "startPosition: " << startPosition << "\tread length: " << read1Length << endl;
// 			fixedSequences[fixedSequence] = startPosition;
// 			if (startPosition < 0) {
// 				string fixedSequenceRC = reverseComplement(fixedSequence);
// 				fixedSequences[fixedSequenceRC] = startPosition;
// 			}
// 		}
// 	}
// 	fixedFileStream.close();
// 	cerr << "Get fixed sequences: " << fixedSequences.size() << endl;
// }
