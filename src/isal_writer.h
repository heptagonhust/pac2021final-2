#pragma once
#include <thread>

#include "options.h"
#include "ringbuf.hpp"
#include "writer.h"

#define ISAL_BLOCK_SIZE 32 * 1024 * 1024
#define ISAL_OUTPUT_BLOCK_SIZE (ISAL_BLOCK_SIZE * 2)
/*
struct WriterInput {
  uint8_t *str;
  size_t size;
};
*/
typedef RingBuf<BufferedChar*> WriterInputRB;

class IsalWriter {
public:
 
  IsalWriter(const Options* opt, const string& filename, WriterInputRB *inputs);
  ~IsalWriter();

  void join();
private:

  struct IsalTask {
    BufferedChar* buffer;
    uint8_t output[ISAL_OUTPUT_BLOCK_SIZE];
    size_t output_size;
    IsalTask() : output_size(0) {};
  };

  int fd;
  RingBuf<IsalTask> **worker_inputs;
  std::thread **threads;
  thread *main;
  
  void mainTask(WriterInputRB *inputs, int nInputs);
  void otherTask(int id);

  void single(WriterInputRB *inputs, int nInputs);
};