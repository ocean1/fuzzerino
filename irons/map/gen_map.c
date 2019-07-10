#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../../afl/config.h"

#define MAX_WHITELIST 1024

int main(int argc, char *argv[]) {

    int whitelist_all = 0;

    if (argc < 2) {
        printf("\nargc < 2. Please specify the number of locations to instrument.\n");
        return -1;
    }

    if (argc == 3 && !strcmp(argv[2], "-w")) {
        whitelist_all = 1;
    }

    // Setup memory
    uint16_t *__gfz_map_area = NULL;

    __gfz_map_area = calloc(GFZ_MAP_SIZE, 1);

    // Parse arg
    int locations = strtol(argv[1], 0, 10);

    if (whitelist_all) {

        int i = 0;
        for (i = 0; i < locations; ++i) {
            __gfz_map_area[i] = GFZ_KEEP_ORIGINAL;
        }

    } else {

        // Get random data
        FILE *rfd;

        rfd = fopen("/dev/urandom", "r");

        if (fread(__gfz_map_area, locations * 2, 1, rfd) != 1) {
            printf("[-] Unable to get enough rand bytes");
        }
        
        fclose(rfd);

        // Get whitelisted locations and set them to KEEP_ORIGINAL
        FILE *wfd;

        wfd = fopen("gfz.whitelist", "r");

        int *whitelist = malloc(MAX_WHITELIST * sizeof(int));
        int cur_loc = 0;

        if (wfd != NULL) {
            printf("\n[*] Using whitelist:");
            while (1) {            
                if (fscanf(wfd, "%d", &whitelist[cur_loc]) != 1)
                    break;

                cur_loc++;
            }
            printf(" read %d locations.", cur_loc);
        } else {
            printf("\n[*] No whitelist.");
        }

        int k = 0;
        for (int k=0; k<cur_loc; ++k) {
            __gfz_map_area[whitelist[k]] = GFZ_KEEP_ORIGINAL;
        }

    }

    // Write data to map file
    FILE *map_file;

    if (!(map_file = fopen("./gfz.map", "wb")))
    {
        printf("Error! Could not open file\n");
        return -1;
    }
    
    ssize_t n_bytes = fwrite(__gfz_map_area, GFZ_MAP_SIZE, 1, map_file);

    if (n_bytes <= 0) {
        printf("ko (%ld)\n", n_bytes);
        return -1;
    }
    else
        printf("\n[+] Map written.\n");

    return 0;

}
