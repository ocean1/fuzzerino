#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gfuzz.h>

__fuzz int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("\nargc != 4\n");
        return -1;
    }

    int a, b, c;

    a = strtol(argv[1], 0, 10);
    b = strtol(argv[2], 0, 10);
    c = strtol(argv[3], 0, 10);

    printf("\nI read: a = %d, b = %d, c = %d", a, b, c);

    printf("\na + b = %d", a + b);
    printf("\na + c = %d", a + c);
    printf("\nb + c = %d", b + c);
    printf("\na - b = %d", a - b);
    printf("\na * c = %d", a * c);
    printf("\nb & c = %d", b & c);

    printf("\nGoodbye!\n");

    return 0;

}