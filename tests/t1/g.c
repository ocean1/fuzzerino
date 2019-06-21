/* basic example of generator 1
 * more complex example will build on this
 * to "simulate" a basic binary file */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <gfuzz.h>

const char* magic = "FFT"; // fast fuzzer test
const int magic_len = 4;

// default content if string hasn't been passed via cmdline
char* dcontent = "THIS IS A G-FUZZING TEST BEWARE";

void writefile(uint8_t *buf, uint32_t size){
	FILE* fout;
	fout = fopen("/dev/shm/fuzztest", "w"); // output file

	fwrite(buf, size, 1, fout);
}

__fuzz void createfmt(char *content){
	uint32_t len = strlen(content);
	uint32_t fsize = 2 * sizeof(uint32_t) + len;
	uint8_t *obuf, *buf;

	obuf = buf = malloc(fsize);
	
	memcpy(obuf, magic, sizeof(uint32_t));
	obuf += sizeof(uint32_t);
	memcpy(obuf, &len, sizeof(uint32_t));
	obuf += sizeof(uint32_t);
	memcpy(obuf, content, len);

	writefile(buf, fsize);
}

int main(int argc, char **argv)
{
	char *content;

	if (argc > 1){
		content = argv[1];
	} else {
		content = dcontent;
	}
	
	createfmt(content);
}
