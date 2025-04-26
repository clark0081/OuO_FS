#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
    int err;
    //int fd;
    int nch;
    int status;
    //int se = 0;
    char ch;
    //char ch;
    printf("QQ\n");
    write(2, "A\n", 2);
    err = Create("/footest3");
    fprintf(stderr, "Create returned %d\n", err);
    
    for(int i = 0; i < 1; i++){
        nch = Write(err, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 5);
        //printf("Write nch %d\n", nch);
    }
    Seek(err, 0, SEEK_SET);
    for(int i = 0; i < 5; i++){
        nch = Read(err, &ch, 1);
        printf("ch 0x%x '%c'\n", ch, ch);
    }
    /*
    while (1) {
        nch = Read(fd, &ch, 1);
        printf("Read nch %d\n", nch);
        if (nch <= 0)
            break;
        printf("ch 0x%x '%c'\n", ch, ch);
    }
    */
    

    Seek(err, 3, SEEK_SET);
    printf("seek 3\n");
    nch = Write(err,"bbbbbbb",5);
    printf("write 5 b\n");
    Seek(err, 0, SEEK_SET);
    printf("seek 0\n");
    for(int i = 0; i < 5; i++){
        nch = Read(err, &ch, 1);
        printf("ch 0x%x '%c'\n", ch, ch);
    }

    Seek(err, 15, SEEK_SET);
    nch = Write(err,"ccccccc",6);
    Seek(err, 0, SEEK_SET);
    for(int i = 0; i < 25; i++){
        nch = Read(err, &ch, 1);
        printf("ch 0x%x '%c'\n", ch, ch);
    }


    status = Close(nch);
    printf("Close status %d\n", status);
    /*
    int fd;
    int nch;
    int status;
    char ch;

    fd = Create("/xxxxxx");
    printf("Create fd %d\n", fd);

    nch = Write(fd, "abcdefghijklmnopqrstuvwxyz", 26);
    printf("Write nch %d\n", nch);

    nch = Write(fd, "0123456789", 10);
    printf("Write nch %d\n", nch);

    status = Close(fd);
    printf("Close status %d\n", status);

    Sync();

    fd = Open("/xxxxxx");
    printf("Open fd %d\n", fd);

    while (1) {
	nch = Read(fd, &ch, 1);
	printf("Read nch %d\n", nch);
	if (nch <= 0)
	    break;
	printf("ch 0x%x '%c'\n", ch, ch);
    }

    status = Close(fd);
    printf("Close status %d\n", status);

    Shutdown();
    */

    /*
    int status;
	static char buffer[1024];
	struct Stat sb;
	int fd;

	status = Create("/a");
	printf("Create status %d\n", status);

	status = SymLink("/a", "/b");
	printf("SymLink status %d\n", status);

	status = ReadLink("/b", buffer, sizeof(buffer));
	printf("ReadLink status %d\n", status);
	printf("link = '%s'\n", buffer);

	status = Stat("/a", &sb);
	printf("Stat status %d\n", status);
	printf("/a: inum %d type %d size %d nlink %d\n",
	    sb.inum, sb.type, sb.size, sb.nlink);

	status = Stat("/b", &sb);
	printf("Stat status %d\n", status);
	printf("/b: inum %d type %d size %d nlink %d\n",
	    sb.inum, sb.type, sb.size, sb.nlink);

	status = SymLink("/00/11/22/33/44/55/66/77/88/99", "/xxx");
	printf("SymLink status %d\n", status);

	status = SymLink("00/11/22/33/44/55/66/77/88/99", "yyy");
	printf("SymLink status %d\n", status);

	fd = Open("/a");
	printf("Open /a status %d\n", fd); 

	status = Write(fd, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 27);
	printf("Write /a status %d\n", status);

	fd = Open("b");
	printf("Open b status %d\n", fd);

	status = Read(fd, buffer, sizeof(buffer));
	printf("Read b status %d\n", status);
	printf("buffer = '%s'\n", buffer); // 'AB'

	Shutdown();
    
    */

    Sync();
    Delay(3);
    fprintf(stderr, "Done with Sync\n");

	Shutdown(); 

    exit(0);
}
