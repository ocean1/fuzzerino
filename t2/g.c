/* basic example of generator 2
 * let's make things more interesting, add binary blob(s)
 * on top and an array of struct  (let's see with arrays what we get)
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

const char* magic = "FFT"; // fast fuzzer test
const int magic_len = 4;

// default content if string hasn't been passed via cmdline
uint8_t* dcontent = (uint8_t*)"THIS IS A G-FUZZING TEST BEWARE";

typedef struct chunkstruct {
	uint32_t chunksize;
	uint8_t *content;
} chunk;

typedef struct fmtstruct {
	uint32_t magic;
	uint32_t cmt_len;	// comment string
	uint32_t chunksnum;
	uint8_t *cmt;
} fmt;

void writefile(uint8_t *buf, uint32_t size){
	FILE* fout;
	fout = fopen("fuzztest", "w"); // output file
	

	fwrite(buf, size, 1, fout);
}

int main(int argc, char **argv)
{
	uint8_t *content;

	fmt h;
	chunk *c;

	if (argc > 1){
		content = (uint8_t *)argv[1];
	} else {
		content = dcontent;
	}
	
	h.magic = *(uint32_t*)magic;
	h. cmt_len = strlen((char*)content);

}
