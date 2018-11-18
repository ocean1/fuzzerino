/* simple parser (1) responds to g.c format */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

const char* magic = "FFT"; // fast fuzzer test
const int magic_len = 4;
char* gcontent = "THIS IS A G-FUZZING TEST BEWARE";


bool readstatic(FILE* fd, uint8_t **content, uint32_t *len){
	void* buf = *content;
	
	fread(buf, sizeof(uint32_t), 1, fd);
	if (!strcmp(buf, magic)){
		return false;
	}
	buf += sizeof(uint32_t);

	fread(len, sizeof(uint32_t), 1, fd);

	buf += sizeof(uint32_t);

	fread(buf, *len, 1, fd);
	
	return true;
}

char g_content[512];

int main(int argc, char **argv)
{
	FILE* fin;
	char l_content[512];
	bool res = false;
	uint32_t len;
	uint8_t test = 0;

	if (argc > 1) {
		test = (uint8_t)atoi(argv[1]);
	}

	fin = fopen("fuzztest", "r"); // output file
	
	switch (test) {
		case 0:
			res = readstatic(fin, (uint8_t **)&l_content, &len);
			break;
		case 1:
			res = readstatic(fin, (uint8_t **)&g_content, &len);
			break;
		case 2:
		default: break;
	}

	printf("result %s", res ? "true": "false");
}
