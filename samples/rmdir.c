#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
	int status;

	status = RmDir("dir/clark/.");
	printf("Rmdir status %d\n", status);

	status = RmDir("dir/clark/..");
	printf("Rmdir status %d\n", status);

    status = RmDir("dir/clark/c2");
	printf("Rmdir status %d\n", status);

    status = RmDir("dir/../dir/clark");
	printf("Rmdir status %d\n", status);

	Shutdown();
}
