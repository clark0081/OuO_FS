#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
	int status;

    status = MkDir("sc_d1");
    printf("mkdir status %d\n", status);

    status = MkDir("sc_d2");
    printf("mkdir status %d\n", status);

    status = SymLink("../sc_d2/sc_f1","sc_d1/sc_l1");
    printf("sym status %d\n", status);

    status = Create("sc_d1/sc_l1");
    printf("Create status %d\n", status);

    status = SymLink("../sc_d2/sc_d3","sc_d1/sc_l2");
    printf("sym status %d\n", status);
    
    status = MkDir("sc_d2/sc_d3");
    printf("mkdir status %d\n", status);

    status = ChDir("sc_d1/sc_l2");
    printf("chdir status %d\n", status);

    status = Create("sc_f2");
    printf("Create status %d\n", status);


	Shutdown();
}
