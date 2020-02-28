#include <Windows.h>
#include <stdio.h>

int main() {
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	while (1) {
		printf("working...\n");
		Sleep(500);
	}
}