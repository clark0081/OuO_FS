#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
	int status;

    status = MkDir("clark/c1");
	printf("Mkdir status %d\n", status);

    status = MkDir("dir/clark/c2");
	printf("Mkdir status %d\n", status);

	status = ChDir("/dir");
	printf("ChDir status %d\n", status);

	status = Create("antony");
	printf("Create status %d\n", status);

    status = MkDir("antony");
	printf("Mkdir status %d\n", status);

    status = MkDir("clark");
	printf("Mkdir status %d\n", status);

    status = MkDir("/clark");
	printf("Mkdir status %d\n", status);

    status = MkDir("clark/c2");
	printf("Mkdir status %d\n", status);

    // status = RmDir("/clark");
	// printf("Rmdir status %d\n", status);

    // status = RmDir("clark/c2");
	// printf("Rmdir status %d\n", status);


	Shutdown();
}
