#pragma once
#include "read.h"
#include "common.h"

#include <semaphore.h>

class IReadPair{
public:
    IReadPair();
    ~IReadPair();

public:
    Read mLeft;
    Read mRight;
};


struct ReadPairPack {
	IReadPair data[PACK_SIZE];
	int count_left;
	int count_right;
	ReadPairPack() {
		count_left = count_right = 0;
	}
};

class RingBufPair
{
private:
  size_t start = 0;
  size_t end_left = 0;
  size_t end_right = 0;

  sem_t empty_left, empty_right;
  sem_t full_left, full_right;

  ReadPairPack* bufs;
public:
  const size_t capacity;
  RingBufPair();
  RingBufPair(size_t capacity);
  ~RingBufPair();

  ReadPairPack* enqueue_acquire_left();
  void enqueue_left();
  ReadPairPack* enqueue_acquire_right();
  void enqueue_right();

  ReadPairPack* dequeue_acquire();
  void dequeue();
};