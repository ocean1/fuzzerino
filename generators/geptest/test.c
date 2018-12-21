#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

int main(){
	uint8_t lol[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\x00";

	bool l;
	uint32_t a = ((uint32_t*)lol)[0];

	printf("a = %x", a);
	l = (bool)a;
	return l;
}
