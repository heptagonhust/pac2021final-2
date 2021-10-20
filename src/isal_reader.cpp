#include "isa-l.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "isal_reader.h"

IsalReader::IsalReader(const char* filename, ReaderOutputRB *output, uint8_t* dst)
{

  f = fopen(filename, "rb");
  if(f == NULL)
    error_exit("failed to open output file");
}

IsalReader::~IsalReader() {
  fclose(f);
}

#define ISAL_BLOCK_SIZE 32 * 1024 * 1024

size_t IsalReader::task(ReaderOutputRB *output, uint8_t* dst) {
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
	state.avail_in = fread(state.next_in, 1, inbuf_size, f);

  // Actually read and save the header info
	ret = isal_read_gzip_header(&state, &gz_hdr);
	if (ret != ISAL_DECOMP_OK) {
		error_exit("igzip: Error invalid gzip header found for file \n");
	}

  size_t total = 0;
  ssize_t n = 0;
  do {
    if(state.avail_in == 0) {
      n = fread(inbuf, 1, inbuf_size, f);
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
    if(n_decompressed > 0) {
      memcpy(&dst[total], outbuf, n_decompressed);
      *output->enqueue_acquire() = n_decompressed;
      output->enqueue();
      total += n_decompressed;
    }


  } while (state.block_state != ISAL_BLOCK_FINISH	// while not done
		 && (!feof(f) || state.avail_out == 0)	// and work to do
	    );

  // Add the following to look for and decode additional concatenated files
	if (!feof(f) && state.avail_in == 0) {
		state.next_in = inbuf;
		state.avail_in = fread(inbuf, 1, inbuf_size, f);
	}

	while (state.avail_in > 0 && state.next_in[0] == 31) {
		// Look for magic numbers for gzip header. Follows the gzread() decision
		// whether to treat as trailing junk
		if (state.avail_in > 1 && state.next_in[1] != 139)
			break;

		isal_inflate_reset(&state);
		state.crc_flag = ISAL_GZIP;	// Let isal_inflate() process extra headers
		do {
			if (state.avail_in == 0 && !feof(f)) {
				state.next_in = inbuf;
				state.avail_in = fread(inbuf, 1, inbuf_size, f);
			}

			state.next_out = outbuf;
			state.avail_out = outbuf_size;

			ret = isal_inflate(&state);
			if (ret != ISAL_DECOMP_OK) {
        error_exit("igzip: Error while decompressing extra concatenated gzip files \n");
			}

      size_t n_decompressed = state.next_out - outbuf;
			if(n_decompressed > 0) {
        memcpy(&dst[total], outbuf, n_decompressed);
        *output->enqueue_acquire() = n_decompressed;
        output->enqueue();
        total += n_decompressed;
      }

		} while (state.block_state != ISAL_BLOCK_FINISH
			 && (!feof(f) || state.avail_out == 0));

		if (!feof(f) && state.avail_in == 0) {
			state.next_in = inbuf;
			state.avail_in = fread(inbuf, 1, inbuf_size, f);
		}
	}

	if (state.block_state != ISAL_BLOCK_FINISH)
		error_exit("igzip: Error does not contain a complete gzip file\n");
	else
		success = 1;
  
  *output->enqueue_acquire() = 0;
  output->enqueue();

  delete inbuf;
  delete outbuf;

  return total;
}
