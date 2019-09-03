#include <stdio.h>
#include "ok_png.h"

int main(int argc, char **argv) {
    FILE *file = fopen(argv[1], "rb");
    ok_png *image = ok_png_read(file, OK_PNG_COLOR_FORMAT_RGBA);
    fclose(file);
    if (image->data) {
        printf("Got image! Size: %li x %li\n", (long)image->width, (long)image->height);
    }
    ok_png_free(image);
    return 0;
}