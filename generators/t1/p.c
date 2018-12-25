/* simple parser (1) responds to g.c format */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

const char* magic = "FFT"; // fast fuzzer test


bool readstatic(FILE* fd, uint8_t *content, uint32_t *len){
	void* buf = content;
	
	if(fread(buf, sizeof(uint32_t), 1, fd) < 1)
		exit(1);

	if (*(uint32_t*)buf != *(uint32_t*)magic) {
		fprintf(stderr, "%x %x", *(uint32_t*)magic, *(uint32_t*)buf);
		return false;
	}
	buf += sizeof(uint32_t);

	fread(len, sizeof(uint32_t), 1, fd);

	buf += sizeof(uint32_t);

	fread(buf, *len, 1, fd);
	fprintf(stderr, "%s ", (char *)buf);
	
	return true;
}

unsigned char g_content[512];

int main(int argc, char **argv)
{
	FILE* fin;
	uint8_t l_content[512];
	bool res = false;
	uint32_t len;
	uint8_t test = 0;

	if (argc > 2) {
		test = (uint8_t)atoi(argv[2]);
	}

	if (argc > 1) {
		fin = fopen(argv[1], "r"); // output file
	} else {
		fin = fopen("/dev/shm/fuzztest", "r");
	}
	
	switch (test) {
		case 0:
			res = readstatic(fin, l_content, &len);
			break;
		case 1:
			res = readstatic(fin, g_content, &len);
			break;
		case 2:
		default: break;
	}

	printf("result %s", res ? "true": "false");
}