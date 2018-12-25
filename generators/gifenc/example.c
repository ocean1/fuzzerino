#include "gifenc.h"

int
main()
{
    int i, j;
    int w = 12, h = 9;
	int p = 2;
	int inf = 0;

    /* create a GIF */
    ge_GIF *gif = ge_new_gif(
        "/dev/shm/fuzztest",  /* file name */
        w, h,           /* canvas size */
        (uint8_t []) {  /* palette */
            0x00, 0x00, 0x00, /* 0 -> black */
            0xFF, 0x00, 0x00, /* 1 -> red */
            0x00, 0xFF, 0x00, /* 2 -> green */
            0x00, 0x00, 0xFF, /* 3 -> blue */
        },
        p,              /* palette depth == log2(# of colors) */
        inf               /* infinite loop */
    );
    /* draw some frames */
    for (i = 0; i < 4*6/3; i++) {
        for (j = 0; j < w*h; j++)
            gif->frame[j] = (i*3 + j) / 6 % 4;
        ge_add_frame(gif, 10);
    }
    /* remember to close the GIF */
    ge_close_gif(gif);
    return 0;
}