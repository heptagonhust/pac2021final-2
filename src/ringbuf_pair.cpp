#include "ringbuf_pair.h"

RingBufPair::RingBufPair()
  : capacity(128)
{
  bufs = new ReadPairPack[capacity];
  sem_init(&empty_left, 0, capacity);
  sem_init(&empty_right, 0, capacity);

  sem_init(&full_left, 0, 0);
  sem_init(&full_right, 0, 0);
}

RingBufPair::RingBufPair(size_t capacity)
  : capacity(capacity)
{
  bufs = new ReadPairPack[capacity];
  sem_init(&empty_left, 0, capacity);
  sem_init(&empty_right, 0, capacity);

  sem_init(&full_left, 0, 0);
  sem_init(&full_right, 0, 0);
}

RingBufPair::~RingBufPair()
{
  delete [] bufs;
  sem_destroy(&empty_left);
  sem_destroy(&empty_right);
  sem_destroy(&full_left);
  sem_destroy(&full_right);
}


ReadPairPack* RingBufPair::enqueue_acquire_left()
{
  sem_wait(&empty_left);
  return &bufs[end_left];
}

void RingBufPair::enqueue_left()
{
  sem_post(&full_left);
  end_left = (end_left + 1) % capacity;
}

ReadPairPack* RingBufPair::enqueue_acquire_right()
{
  sem_wait(&empty_right);
  return &bufs[end_right];
}

void RingBufPair::enqueue_right()
{
  sem_post(&full_right);
  end_right = (end_right + 1) % capacity;
}

ReadPairPack* RingBufPair::dequeue_acquire()
{
  sem_wait(&full_left);
  sem_wait(&full_right);
  return &bufs[start];
}

void RingBufPair::dequeue()
{
  sem_post(&empty_left);
  sem_post(&empty_right);
  start = (start + 1) % capacity;
}

IReadPair::IReadPair()
  : mLeft(), mRight()
{

}

IReadPair::~IReadPair() {

}
