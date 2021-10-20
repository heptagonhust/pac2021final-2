#pragma once
#include <thread>

#include "options.h"
#include "ringbuf.hpp"
#include "writer.h"


typedef RingBuf<size_t> ReaderOutputRB;

class IsalReader {
public:
 
  IsalReader(const Options* opt, const string& filename, ReaderOutputRB *output, uint8_t* dst);
  ~IsalReader();

  void join();
private:
  thread *main;

  int fd;
  void task(ReaderOutputRB *output, uint8_t* dst);
};