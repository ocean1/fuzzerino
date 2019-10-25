/* basic example of generator 2
 * let's make things more interesting, add binary blob(s)
 * on top and an array of struct  (let's see with arrays what we get)
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <gfuzz.h>

#define SEED time(0)

const unsigned char* magic = (unsigned char*)"FFT"; // fast fuzzer test
const int magic_len = 4;

// default content if string hasn't been passed via cmdline
uint8_t* dcontent = (uint8_t*)"THIS IS A G-FUZZING TEST BEWARE\x00";

int randrange(int min, int max){
   return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

typedef struct chunkstruct {
	uint32_t size;
	uint8_t *buf;
	struct chunkstruct* next;
} chunk;

typedef struct fmtstruct {
	uint32_t magic;
	uint32_t cmtlen;	// comment string
	uint8_t *cmt;
	uint32_t chunksnum;
	chunk *chunks;
} fmt;

__fuzz void writefile(fmt h) {
	// we do a pass on the in-memory fmt to "serialize" to output
	// now that's were we need to start watching out for crazy
	// crash conditions, also, we'll need to build a format converter
	// that's were we might crash like crazy, we can look at one strategy
	// where we "disable" whatever crashes out approach sound approach,
	// potentially good for building aflfuzz seeds
	// and another were we instead select manually with PRAGMA what should 
	// be fuzzed(?) not really convincing.... :P
	FILE* fd;
	fd = fopen("/dev/shm/fuzztest", "w"); // output file
	chunk *c;

	fwrite(&h.magic, sizeof(uint32_t), 1, fd);
	fwrite(&h.cmtlen, sizeof(uint32_t), 1, fd);
	fwrite(&h.chunksnum, sizeof(uint32_t), 1, fd);
	fwrite(h.cmt, h.cmtlen, 1, fd);
	for (c = h.chunks; c!=NULL; (c=c->next)) {
		fwrite(c->buf, c->size, 1, fd);
	}

}

__fuzz int main(int argc, char **argv)
{
	int i, cnum;
	uint32_t rsize;
	fmt h;
	chunk tc, *hc, *nc;
	tc.next = NULL;
	hc = &tc;
	h.cmt = dcontent;

	srand(SEED);
	cnum = randrange(0, 10);

	if (argc > 1){
		h.cmt = (uint8_t *)argv[1];
	}
	if (argc > 2){
		cnum = atoi(argv[2]);
	}
	
	h.magic = *(uint32_t*)magic;
	h.cmtlen = strlen((char*)h.cmt);
	
	for (i=0; i<cnum; i++){
		
		rsize = randrange(0, 1024);

		hc->buf = malloc(rsize);
		memset(hc->buf, i, rsize);
		hc->size = rsize;

		nc = malloc(sizeof(chunk));
		nc->next = hc;
		hc = nc;
	}

	h.chunks = hc;

	writefile(h);
}
