#include "isa-l.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "isal_reader.h"

IsalReader::IsalReader(const Options* opt, const string& filename, ReaderOutputRB *output, uint8_t* dst)
{
  if(!ends_with(filename, ".gz"))
    error_exit("output file shuld end with .gz ...");
  

  fd = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
  if(fd == -1)
    error_exit("failed to open output file");

  main = new std::thread(std::bind(&IsalReader::task, this, output, dst));
}

IsalReader::~IsalReader() {
  close(fd);
}

void IsalReader::join() {
  main->join();
}

#define ISAL_BLOCK_SIZE 32 * 1024 * 1024

void IsalReader::task(ReaderOutputRB *output, uint8_t* dst) {
  int fd = this->fd;
  uint8_t *inbuf = NULL, *outbuf = NULL;
	size_t inbuf_size, outbuf_size;
	struct inflate_state state;
	struct isal_gzip_header gz_hdr;

	int ret = 0, success = 0;

  inbuf_size = ISAL_BLOCK_SIZE;
	outbuf_size = ISAL_BLOCK_SIZE;
  inbuf = new uint8_t[inbuf_size];
	outbuf = new uint8_t[outbuf_size];

  isal_gzip_header_init(&gz_hdr);
	isal_inflate_init(&state);
	state.crc_flag = ISAL_GZIP_NO_HDR_VER;
	state.next_in = inbuf;
	state.avail_in = read(fd, state.next_in, inbuf_size);

  // Actually read and save the header info
	ret = isal_read_gzip_header(&state, &gz_hdr);
  size_t n_outputed = 0;
	if (ret != ISAL_DECOMP_OK) {
		error_exit("igzip: Error invalid gzip header found for file \n");
	}

  ssize_t n = 0;
  do {
    if(state.avail_in = 0) {
      n = read(fd, state.next_in, inbuf_size);
      state.next_in = inbuf;
      state.avail_in = n;
    }

    state.next_out = outbuf;
		state.avail_out = outbuf_size;

    ret = isal_inflate(&state);
		if (ret != ISAL_DECOMP_OK) {
			error_exit("igzip: Error encountered while decompressing file \n");
		}

    size_t n_decompressed = state.next_out - outbuf;
    memcpy(&dst[n_outputed], outbuf, n_decompressed);
    *output->enqueue_acquire() = n_decompressed;
    output->enqueue();


  } while (state.block_state != ISAL_BLOCK_FINISH	// while not done
		 && (n > 0 || state.avail_out == 0)	// and work to do
	    );

  // Add the following to look for and decode additional concatenated files
	if (n > 0 && state.avail_in == 0) {
    error_exit("igzip: Error concatenated files not supported \n");
	}

  if (state.block_state != ISAL_BLOCK_FINISH) {
    error_exit("igzip: Error does not contain a complete gzip file \n");
  }

  delete inbuf;
  delete outbuf;
}
