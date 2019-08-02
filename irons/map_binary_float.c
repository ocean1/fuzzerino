#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gfuzz.h>

__fuzz int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("\nargc != 4\n");
        return -1;
    }

    float a, b, c;

    a = strtof(argv[1], 0);
    b = strtof(argv[2], 0);
    c = strtof(argv[3], 0);

    printf("\nI read: a = %f, b = %f, c = %f", a, b, c);

    printf("\na + b = %f", a + b);
    printf("\na + c = %f", a + c);
    printf("\nb + c = %f", b + c);
    printf("\na - b = %f", a - b);
    printf("\na * c = %f", a * c);
    printf("\na / c = %f", a / c);

    printf("\nGoodbye!\n");

    return 0;

}