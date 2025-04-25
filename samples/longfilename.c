#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
	int status;
    // expect error
	status = Create("/abcde12345abcde12345abcde12345-"); // 31
	printf("Create status %d\n", status);

	status = Create("/abcde12345abcde12345abcde12345"); // 30
	printf("Create status %d\n", status);

	Shutdown();
}
