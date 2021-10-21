#include "isa-l.h"

#include "isal_writer.h"
#include <cstdio>
#include <unistd.h>
#include <thread>
#include <functional>

#include <fcntl.h>
#include <sys/stat.h>
#include <libaio.h>

const int MAX_IO = 2e8/1024;
io_context_t write_ctx;

const int nWorker = 4;

IsalWriter::IsalWriter(const Options* opt, const string &filename, WriterInputRB *inputs) {
  if(!ends_with(filename, ".gz"))
    error_exit("output file shuld end with .gz ...");
  

  fd = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
  if(fd == -1)
    error_exit("failed to open output file");

  io_setup(MAX_IO, &write_ctx);

  main = new std::thread(std::bind(&IsalWriter::single, this, inputs, opt->thread));

  /*
  worker_inputs = new RingBuf<IsalTask>*[nWorker];
  for(int i=0;i<nWorker;i++) {
    worker_inputs[i] = new RingBuf<IsalTask>(1);
  }
  threads = new std::thread*[nWorker];

  thread *main = new std::thread(std::bind(&IsalWriter::mainTask, this, inputs, opt->thread));
  for(int i=0;i<nWorker;i++) {
    threads[i] = new std::thread(std::bind(&IsalWriter::otherTask, this, i));
  }
  */
}

void IsalWriter::join() {
  /*
  for(int i=0;i<nWorker;i++) {
    threads[i]->join();
  }
  */
 main->join();
}

IsalWriter::~IsalWriter() {
  if(fd != -1)
    close(fd);

  delete main;
  //delete [] worker_inputs;
  //delete [] threads;
}

int compress_level = 3;
const int level_size = ISAL_DEF_LVL3_LARGE;

#define UNIX 3

static bool tryGetInput(WriterInputRB *inputs, bool *inputFinished, int nInputs, BufferedChar *&input, bool &allFinished) {
  allFinished = true;

  for(int i=0;i<nInputs;i++) {
    if(inputFinished[i])
      continue;
    allFinished = false;

    bool sucess = false;
    BufferedChar **p = inputs[i].try_dequeue_acquire(sucess);
    if(sucess) {
      if(!*p)
        inputFinished[i] = true;
      else  {
        input = *p;
        inputs[i].dequeue();
        return true;
      }
    }
  }

  return false;
}

void write_callback(io_context_t ctx, struct iocb *iocb, long res, long res2) {
  delete iocb->data;
}

void IsalWriter::single(WriterInputRB *inputs, int nInputs) {
  bool *inputFinished = new bool[nInputs];
  for(int i=0;i<nInputs;i++)
    inputFinished[i] = false;

	struct isal_zstream stream;
	struct isal_gzip_header gz_hdr;

  uint8_t *level_buf = new uint8_t[level_size];

  isal_gzip_header_init(&gz_hdr);
	gz_hdr.os = UNIX;
  // do not save file name and timestamp in compress

  uint8_t header[32];
  isal_deflate_init(&stream);
  stream.avail_in = 0;
	stream.flush = NO_FLUSH;
	stream.level = compress_level;
	stream.level_buf = level_buf;
	stream.level_buf_size = level_size;
	stream.gzip_flag = IGZIP_GZIP_NO_HDR;
	stream.next_out = header;
	stream.avail_out = 32;

  isal_write_gzip_header(&stream, &gz_hdr);
  int r = write(fd, header, stream.total_out);

  uint32_t crc = 0;
  uint32_t nread = 0;
  int worker_id = 0;
  
  BufferedChar* input;
  uint8_t *outputBuf = new uint8_t[ISAL_OUTPUT_BLOCK_SIZE];
  bool allFinished;
  while(true) {
    if(tryGetInput(inputs, inputFinished, nInputs, input, allFinished)) {

      nread += input->length;
      crc = crc32_gzip_refl(crc, (uint8_t *)input->str, input->length);

      isal_deflate_stateless_init(&stream);

      stream.next_in = (uint8_t*)input->str;
      stream.next_out = outputBuf;
      stream.avail_in = input->length;
      stream.avail_out = ISAL_OUTPUT_BLOCK_SIZE;
      stream.end_of_stream = 0;
      stream.flush = FULL_FLUSH;
      stream.level = compress_level;
      stream.level_buf = level_buf;
      stream.level_buf_size = level_size;

      int r = isal_deflate_stateless(&stream);
      assert(r == COMP_OK);

      write(fd, outputBuf, stream.total_out); // write out previ result
      auto p = new iocb;
      auto &io = *p;
      io_prep_pwrite(&io, fd, outputBuf, stream.total_out, 0);
      io.data = outputBuf;
      io_submit(write_ctx, 1, &p);
      io_set_callback(&io, write_callback);
      delete input;

    } else if(allFinished) {
      
      break;
    }
  }

  io_event e;
  timespec timeout;
  while(1) {
    timeout.tv_sec = 120;
    timeout.tv_sec = 0;
    if(io_getevents(write_ctx, 0, 1, &e, &timeout)==1) {
      break;
    }
    sleep(1);
  }

  // Write gzip trailer
  const char end[] = "\n";
  char end_buff[512];

  crc = crc32_gzip_refl(crc, (uint8_t *)end, sizeof(end) - 1);
  nread += sizeof(end) - 1;
  isal_deflate_stateless_init(&stream);
  stream.next_in = (uint8_t*)end;
  stream.next_out = (uint8_t*)end_buff;
  stream.avail_in = sizeof(end) - 1;
  stream.avail_out = sizeof(end_buff);
  stream.end_of_stream = 1;
  stream.flush = FULL_FLUSH;
  stream.level = compress_level;
  stream.level_buf = level_buf;
  stream.level_buf_size = level_size;
  r = isal_deflate_stateless(&stream);

  write(fd, end_buff, stream.total_out);
  
  write(fd, &crc, sizeof(uint32_t));
  write(fd, &nread, sizeof(uint32_t));
  close(fd);

  delete level_buf;
  delete inputFinished;
}

void IsalWriter::mainTask(WriterInputRB *inputs, int nInputs) {
  bool *inputFinished = new bool[nInputs];
  for(int i=0;i<nInputs;i++)
    inputFinished[i] = false;

	struct isal_zstream stream;
	struct isal_gzip_header gz_hdr;

  uint8_t *level_buf = new uint8_t[level_size];

  isal_gzip_header_init(&gz_hdr);
	gz_hdr.os = UNIX;
  // do not save file name and timestamp in compress

  uint8_t header[32];
  isal_deflate_init(&stream);
  stream.avail_in = 0;
	stream.flush = NO_FLUSH;
	stream.level = compress_level;
	stream.level_buf = level_buf;
	stream.level_buf_size = level_size;
	stream.gzip_flag = IGZIP_GZIP_NO_HDR;
	stream.next_out = header;
	stream.avail_out = 32;

  isal_write_gzip_header(&stream, &gz_hdr);
  int r = write(fd, header, stream.total_out);

  IsalTask * task;
  uint32_t crc = 0;
  uint32_t nread = 0;
  int worker_id = 0;

  task = worker_inputs[worker_id]->enqueue_acquire();
  
  BufferedChar* input;
  bool allFinished;
  while(true) {
    if(tryGetInput(inputs, inputFinished, nInputs, input, allFinished)) {
      task->output_size = 0;
      task->buffer = input;

      nread += input->length;
      crc = crc32_gzip_refl(crc, (uint8_t *)input->str, input->length);

      worker_inputs[worker_id]->enqueue();
      worker_id = (worker_id + 1) % nWorker;
      task = worker_inputs[worker_id]->enqueue_acquire();

      if(task->output_size)
        write(fd, task->output, task->output_size); // write out previ result
      
    } else if(allFinished) {
      task->output_size = 0;
      task->buffer = NULL;
      worker_inputs[worker_id]->enqueue();
      worker_id = (worker_id + 1) % nWorker;
      break;
    }
  }

  // give end_of_stream singal ?

  // flush ringbufs
  for(int i=0;i<nWorker;i++) {
    task = worker_inputs[worker_id]->enqueue_acquire();
    if(task->output_size)
      write(fd, task->output, task->output_size); // write out previ result
    
    task->output_size = 0;
    task->buffer = NULL;
    worker_inputs[worker_id]->enqueue();
    worker_id = (worker_id + 1) % nWorker;
  }
  
  // Write gzip trailer
  const char end[] = "\n";
  char end_buff[512];

  crc = crc32_gzip_refl(crc, (uint8_t *)end, sizeof(end) - 1);
  nread += sizeof(end) - 1;
  isal_deflate_stateless_init(&stream);
  stream.next_in = (uint8_t*)end;
  stream.next_out = (uint8_t*)end_buff;
  stream.avail_in = sizeof(end) - 1;
  stream.avail_out = sizeof(end_buff);
  stream.end_of_stream = 1;
  stream.flush = FULL_FLUSH;
  stream.level = compress_level;
  stream.level_buf = level_buf;
  stream.level_buf_size = level_size;
  r = isal_deflate_stateless(&stream);

  write(fd, end_buff, stream.total_out);
  
  write(fd, &crc, sizeof(uint32_t));
  write(fd, &nread, sizeof(uint32_t));
  close(fd);

  delete level_buf;
  delete inputFinished;
}

void IsalWriter::otherTask(int id) {
	struct isal_zstream stream;
  RingBuf<IsalTask> *input = worker_inputs[id];
  uint8_t *level_buf = new uint8_t[level_size];
  IsalTask *task;
  while(true) {
    bool success = false;
    task = input->dequeue_acquire();
    if(task->buffer == NULL){
      task->output_size = 0;
      input->dequeue();
      break;
    }
    isal_deflate_stateless_init(&stream);
    stream.next_in = (uint8_t*)task->buffer->str;
    stream.next_out = task->output;
    stream.avail_in = task->buffer->length;
    stream.avail_out = ISAL_OUTPUT_BLOCK_SIZE;
    stream.end_of_stream = 0;
    stream.flush = FULL_FLUSH;
    stream.level = compress_level;
    stream.level_buf = level_buf;
    stream.level_buf_size = level_size;
    int r = isal_deflate_stateless(&stream);
    assert(r == COMP_OK);

    delete task->buffer;
    task->output_size = stream.total_out;
    input->dequeue();
  }

  delete level_buf;
} 

