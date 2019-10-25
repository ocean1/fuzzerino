#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define __fuzz __attribute__((section(".fuzzables")))

__fuzz int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("argc != 4");
        return -1;
    }

    float a, b, c;

    a = strtof(argv[1], 0);
    b = strtof(argv[2], 0);
    c = strtof(argv[3], 0);

    printf("%f",  a);
    printf(" %f", b);
    printf(" %f", c);
    printf(" %f", a + b);
    printf(" %f", a + c);
    printf(" %f", b + c);
    printf(" %f", a - b);
    printf(" %f", a * c);
    printf(" %f", b / c);

    return 0;

}