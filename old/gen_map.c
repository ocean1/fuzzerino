#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../afl/config.h"

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("\nargc < 3. Please specify the number of locations to instrument and map type."
               "\n          (-n num, -p ptr, -b branch)\n");
        return -1;
    }

    // Setup memory
    size_t size;
    char *filename;

    if (!strcmp(argv[2], "-n")) {
        
        size = GFZ_NUM_MAP_SIZE;
        filename = "./gfz_num";

        uint16_t *__gfz_map_area = calloc(size, 1);

        // Parse arg
        int locations = strtol(argv[1], 0, 10);

        int i = 0;
        for (i = 0; i < locations; ++i) {
            __gfz_map_area[i] = GFZ_KEEP_ORIGINAL;
        }

        // Write data to map file
        FILE *map_file;

        if (!(map_file = fopen(filename, "wb")))
        {
            printf("Error! Could not open file\n");
            return -1;
        }
        
        ssize_t n_bytes = fwrite(__gfz_map_area, size, 1, map_file);

        if (n_bytes <= 0) {
            printf("ko (%ld)\n", n_bytes);
            return -1;
        }
        else
            printf("\n[+] Map written.\n");

        return 0;

    } else if (!strcmp(argv[2], "-p")) {
        
        size = GFZ_PTR_MAP_SIZE;
        filename = "./gfz_ptr";

        uint16_t *__gfz_map_area = calloc(size, 1);

        // Parse arg
        int locations = strtol(argv[1], 0, 10);

        int i = 0;
        for (i = 0; i < locations; ++i) {
            __gfz_map_area[i] = GFZ_KEEP_ORIGINAL;
        }

        // Write data to map file
        FILE *map_file;

        if (!(map_file = fopen(filename, "wb")))
        {
            printf("Error! Could not open file\n");
            return -1;
        }
        
        ssize_t n_bytes = fwrite(__gfz_map_area, size, 1, map_file);

        if (n_bytes <= 0) {
            printf("ko (%ld)\n", n_bytes);
            return -1;
        }
        else
            printf("\n[+] Map written.\n");

        return 0;

    } else if (!strcmp(argv[2], "-b")) {
        
        size = GFZ_BNC_MAP_SIZE;
        filename = "./gfz_bnc";

        uint8_t *__gfz_map_area = calloc(size, 1);

        // Parse arg
        int locations = strtol(argv[1], 0, 10);

        int i = 0;
        for (i = 0; i < locations; ++i) {
            __gfz_map_area[i] = GFZ_KEEP_ORIGINAL;
        }

        // Write data to map file
        FILE *map_file;

        if (!(map_file = fopen(filename, "wb")))
        {
            printf("Error! Could not open file\n");
            return -1;
        }
        
        ssize_t n_bytes = fwrite(__gfz_map_area, size, 1, map_file);

        if (n_bytes <= 0) {
            printf("ko (%ld)\n", n_bytes);
            return -1;
        }
        else
            printf("\n[+] Map written.\n");

        return 0;

    } else {
        printf("\ninvalid map type: %s\n", argv[2]);
        return -1;
    }

}
