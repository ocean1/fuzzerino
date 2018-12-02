#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define TJE_IMPLEMENTATION
#include "tiny_jpeg.h"

int main(int argc, char *argv[]){

	const int num_components = 4, width=10, height=10;

	const unsigned int dsize = width*height*num_components;
	uint8_t data[dsize];

if (argc>1){
	memset(data, atoi(argv[1]), sizeof(data));
} else {
	FILE *fio = fopen("/dev/urandom", "r");

	fread(data, dsize, 1, fio);

	fclose(fio);
}

	tje_encode_to_file("/dev/shm/fuzztest", width, height, num_components, data);

	return 0;
}
