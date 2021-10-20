#pragma once
#include <thread>
#include <cstdio>

#include "options.h"
#include "ringbuf.hpp"
#include "writer.h"


typedef RingBuf<size_t> ReaderOutputRB;

class IsalReader {
public:
 
  IsalReader(const char* filename, ReaderOutputRB *output, uint8_t* dst);
  ~IsalReader();

  size_t task(ReaderOutputRB *output, uint8_t* dst);
private:

  FILE* f;
};