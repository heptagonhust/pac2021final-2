#pragma once

#include <semaphore.h>
#include <errno.h>
#define RINGBUF_MAX_BUFFER_LENGTH 1ll<<25

// one to one only
template <typename T> 
class RingBuf
{
private:
  size_t start = 0;
  size_t end = 0;
  sem_t empty;
  sem_t full;

  T* bufs;
public:
  const size_t capacity;
  RingBuf();
  RingBuf(size_t capacity);
  ~RingBuf();

  T* enqueue_acquire();
  void enqueue();

  T* dequeue_acquire();
  T* try_dequeue_acquire(bool &success);
  void dequeue();
};

template <typename T> 
RingBuf<T>::RingBuf()
  : capacity(RINGBUF_MAX_BUFFER_LENGTH)
{
  bufs = new T[capacity];
  sem_init(&empty, 0, capacity);
  sem_init(&full, 0, 0);
}


template <typename T> 
RingBuf<T>::RingBuf(size_t capacity)
  : capacity(capacity)
{
  bufs = new T[capacity];
  sem_init(&empty, 0, capacity);
  sem_init(&full, 0, 0);
}

template <typename T> 
RingBuf<T>::~RingBuf()
{
  delete [] bufs;
  sem_destroy(&empty);
  sem_destroy(&full);
}

template <typename T> 
T* RingBuf<T>::enqueue_acquire()
{
  sem_wait(&empty);
  return &bufs[end];
}

template <typename T> 
void RingBuf<T>::enqueue()
{
  sem_post(&full);
  end = (end + 1) % capacity;
}

template <typename T> 
T* RingBuf<T>::dequeue_acquire()
{
  sem_wait(&full);
  return &bufs[start];
}

template <typename T> 
T* RingBuf<T>::try_dequeue_acquire(bool &success)
{
  int r = sem_trywait(&full);
  success = r == 0;
  
  return &bufs[start];
}


template <typename T> 
void RingBuf<T>::dequeue()
{
  sem_post(&empty);
  start = (start + 1) % capacity;
}
