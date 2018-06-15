#ifndef _FOR_SLAVE_H
#define _FOR_SLAVE_H
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/mman.h>
	#include <sys/types.h>
	#include "useful.h"

void print_file_size(size_t file_size, size_t disk_file_size){
	PRINT("file size %u  disk size %u\n", file_size, disk_file_size);
}

void munmap_for_read(char *mmap_buffer, size_t count){
	if(munmap(mmap_buffer, count) == -1) perror_exit("Failed to munmap for read content: ", 1);
}

size_t mmap_read(int fd, char **mmap_buffer){
	size_t ret, *receive;
	if((receive = (size_t *)mmap(NULL, sizeof(size_t), PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED)
		perror_exit("Failed to mmap for read length: ", 1);
	if((ret = *receive) == 0){
		#ifdef DEBUG
			PRINT("EOF\n");
		#endif
		return 0;
	}
	#ifdef DEBUG
		else if(ret < 0){ PRINT("Weird receive length\n"); exit(1);}
		else PRINT("received %d\n", ret);
	#endif
	if(munmap(receive, sizeof(size_t)) == -1) perror_exit("Failed to munmap for read length: ", 1);
	
	if((*mmap_buffer = (char *)mmap(NULL, ret, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED)
		perror_exit("Failed to mmap for read content: ", 1);
	#ifdef DEBUG
		PRINT("read %u\n", count);
		#ifdef CONTENT
			write(STDOUT_FILENO, mmap_buffer, count);
		#endif
	#endif
	return ret;
}

void mmap_write(size_t *file_size, size_t *disk_file_size, int fd, char *buf, size_t count){
	size_t fs = *file_size, dfs = *disk_file_size;
	char *mmap_buffer;
	if(posix_fallocate(fd, dfs, fs + PAGE_SIZE - dfs) != 0)	perror_exit("Failed to fallocate: ", 1);
	*disk_file_size = (fs + PAGE_SIZE);
	if((mmap_buffer = (char *)mmap(NULL, PAGE_SIZE, PROT_WRITE|PROT_READ|PROT_EXEC, MAP_SHARED, fd, fs)) == MAP_FAILED)
		perror_exit("Failed to mmap for write: ", 1);
	if(memcpy(mmap_buffer, buf, count) == NULL) perror_exit("Failed to memcpy: ", 1);
	*file_size += count;
	#ifdef DEBUG
		PRINT(stdout, "wrote %u   ", count); print_file_size(*file_size, *disk_file_size);
		#ifdef CONTENT
			write(STDOUT_FILENO, mmap_buffer);
		#endif
	#endif
	if(munmap(mmap_buffer, PAGE_SIZE) == -1) perror_exit("Failed to munmap for write: ", 1);
}

#endif
