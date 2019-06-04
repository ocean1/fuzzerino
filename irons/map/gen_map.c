#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define KEEP_ORIGINAL 1
#define PLUS_ONE 2
#define MINUS_ONE 4
#define PLUS_MAX 8
#define PLUS_RAND 16

int whitelist[] = { 0, 1 };

int is_in_whitelist(int id) {

    int i = 0;
    for (i = 0; i < sizeof(whitelist); ++i) {
        if (id == whitelist[i])
            return 1;
    }

    return 0;

}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("\nko\n");
        return -1;
    }

    uint32_t *__gfz_map_area = NULL;
    const ssize_t __gfz_map_size = 4 * 1024 * 1024;

    __gfz_map_area = calloc(__gfz_map_size, 1);

    int locations = strtol(argv[1], 0, 10);

    srand(time(NULL));

    int i, j = 0;
    for (i = 0; i < locations; ++i) {

        uint32_t mutations = 0;

        if (is_in_whitelist(i)) {
            mutations = 1;
        } else {
            int n = (rand() % 5) + 1;

            for (j = 0; j < n; ++j) {
                uint32_t chosen = rand() % 5;

                switch (chosen) {
                    case 0:
                        chosen = KEEP_ORIGINAL;
                        break;
                    case 1:
                        chosen = PLUS_ONE;
                        break;
                    case 2:
                        chosen = MINUS_ONE;
                        break;
                    case 3:
                        chosen = PLUS_MAX;
                        break;
                    case 4:
                        chosen = PLUS_RAND;
                        break;
                }

                mutations |= chosen;
            }
        }

        __gfz_map_area[i] = mutations;
    }

    FILE *map_file;

    if (!(map_file = fopen("./gfz.map", "wb")))
    {
        printf("Error! Could not open file\n");
        return -1;
    }
    
    ssize_t n_bytes = fwrite(__gfz_map_area, __gfz_map_size, 1, map_file);

    if (n_bytes <= 0) {
        printf("ko (%ld)\n", n_bytes);
        return -1;
    }
    else
        printf("[+] Map written.\n");

    return 0;

}