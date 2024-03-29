#ifndef CHIPMASKHDF5_H
#define CHIPMASKHDF5_H

#include <iostream>
#include <hdf5.h>
#include <unordered_map>
#include <thread>
#include <memory>
#include "common.h"
#include "util.h"

#define RANK 3
#define CDIM0 1537
#define CDIM1 1537
#define DATASETNAME "bpMatrix_"
#define ATTRIBUTEDIM 6   //rowShift, colShift, barcodeLength, slide pitch
#define ATTRIBUTEDIM1 2
#define ATTRIBUTENAME "dnbInfo"
#define ATTRIBUTENAME1 "chipInfo"

using namespace std;

#define LOAD_THREADS 16

class ChipMaskHDF5{
public:
    ChipMaskHDF5(std::string FileName);
    ~ChipMaskHDF5();

    void creatFile();
    herr_t writeDataSet(std::string chipID, slideRange& sliderange, BarcodeMap& bpMap, uint32_t BarcodeLen, uint8_t segment, uint32_t slidePitch, uint compressionLevel = 6, int index = 1);
    void openFile();
    void readDataSet(BarcodeMap& bpMap, int index = 1); 

public:
    std::string fileName;
    hid_t fileID;
    uint64*** bpMatrix;
};

#endif 
