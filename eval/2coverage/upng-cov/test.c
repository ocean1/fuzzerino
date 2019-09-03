#include <stdio.h>
#include <malloc.h>

#include "upng.h"

int main(int argc, char** argv) {
	upng_t* upng;

	if (argc <= 1) {
		return 0;
	}

	upng = upng_new_from_file(argv[1]);
	upng_decode(upng);
	if (upng_get_error(upng) != UPNG_EOK) {
		printf("error: %u %u\n", upng_get_error(upng), upng_get_error_line(upng));
		return 0;
	}

	upng_get_width(upng);
	upng_get_height(upng);
	upng_get_components(upng);
	upng_get_buffer(upng);
	upng_free(upng);
	return 0;
}
