#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
	int status;
    status =MkDir("/old1");
    printf("mkdir status %d\n", status);

    status =MkDir("/new1");
    printf("mkdir status %d\n", status);

    status =MkDir("/new1/new2");
    printf("mkdir status %d\n", status);

	status = Create("/old1/old2_f");
	printf("Create status %d\n", status);

	status = Link("/old1/old2_f", "/new1/new3"); // ok
	printf("Link status %d\n", status);

    status = Link("/old1/old2_f", "/new1/new2/.");
    printf("Link status %d\n", status);

    status = Link("/old1/old2_f", "/new1/new2/new3"); // ok
    printf("Link status %d\n", status);

	Shutdown();
}
