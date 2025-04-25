#include <stdio.h>

#include <comp421/yalnix.h>
#include <comp421/iolib.h>

int
main()
{
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

	status = ReadLink("/b", buffer, sizeof(buffer));
	printf("ReadLink status %d\n", status);
	printf("1 link = '%s'\n", buffer); // link = '/a'

	status = Stat("/b", &sb);
	printf("Stat status %d\n", status);
	printf("/b: inum %d type %d size %d nlink %d\n",
	    sb.inum, sb.type, sb.size, sb.nlink);
	
	status = ReadLink("/b", buffer, sizeof(buffer));
	printf("ReadLink status %d\n", status);
	printf("2 link = '%s'\n", buffer); // 2 link = '/a'

	status = SymLink("/00/11/22/33/44/55/66/77/88/99", "/xxx");
	printf("SymLink status %d\n", status);

	status = SymLink("00/11/22/33/44/55/66/77/88/99", "yyy");
	printf("SymLink status %d\n", status);

	status = ReadLink("/b", buffer, sizeof(buffer));
	printf("ReadLink status %d\n", status);
	printf("3 link = '%s'\n", buffer); // 3 link = '00'

	fd = Open("/a");
	printf("Open /a status %d\n", fd); 

	status = ReadLink("/b", buffer, sizeof(buffer)); 
	printf("ReadLink status %d\n", status);
	printf("4 link = '%s'\n", buffer); // 4 link = '00'

	status = Write(fd, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 27);
	printf("Write /a status %d\n", status);

	status = ReadLink("/b", buffer, sizeof(buffer));
	printf("ReadLink status %d\n", status);
	printf("5 link = '%s'\n", buffer); // 5 link = 'AB'

	fd = Open("b");
	printf("Open b status %d\n", fd);

	status = ReadLink("/b", buffer, sizeof(buffer));
	printf("ReadLink status %d\n", status);
	printf("link = '%s'\n", buffer);

	status = Read(fd, buffer, sizeof(buffer));
	printf("Read b status %d\n", status);
	printf("buffer = '%s'\n", buffer); // 'AB'

	Shutdown();
}
