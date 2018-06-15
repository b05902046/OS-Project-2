#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

int main(){
	int fd = open("test.txt", O_RDWR|O_CREAT|O_TRUNC);
	if(fd < 0){
		perror("Failed to open: "); exit(1);
	}
	if(posix_fallocate(fd, 0, 512) != 0){
		perror("Failed to fallocate: "); exit(1);
	}
	char *map = (char *)mmap(NULL, 512, PROT_WRITE|PROT_READ|PROT_EXEC, MAP_SHARED, fd, 0);
	if(map == MAP_FAILED){
		perror("Failed to mmap: "); exit(1);
	}
	sprintf(map, "SHIT");
	exit(0);
}
