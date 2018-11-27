#include <stdio.h>
#include <stdint.h>

int main(){
	uint8_t lol[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\x00";

	uint32_t a = ((uint32_t*)lol)[0];

	printf("a = %x", a);
	return a;
}
