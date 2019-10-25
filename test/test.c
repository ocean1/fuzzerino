#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define __fuzz __attribute__((section(".fuzzables")))

__fuzz int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("argc != 4");
        return -1;
    }

    int a, b, c;

    a = strtol(argv[1], 0, 10);
    b = strtol(argv[2], 0, 10);
    c = strtol(argv[3], 0, 10);

    printf("%d",  a);
    printf(" %d", b);
    printf(" %d", c);
    printf(" %d", a + b);
    printf(" %d", a + c);
    printf(" %d", b + c);
    printf(" %d", a - b);
    printf(" %d", a * c);
    printf(" %d", b / c);

    return 0;

}