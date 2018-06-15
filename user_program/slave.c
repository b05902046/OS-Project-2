#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include "useful.h"

size_t PAGE_SIZE;
#define BUF_SIZE 512
int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size = 0, data_size = -1;
	char file_name[50];
	char method[20];
	char ip[20];
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	char *kernel_address, *file_address;


	strcpy(file_name, argv[1]);
	strcpy(method, argv[2]);
	strcpy(ip, argv[3]);

	if( (dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
	{
		perror("failed to open /dev/slave_device\n");
		return 1;
	}
	gettimeofday(&start ,NULL);
	if( (file_fd = open (file_name, O_RDWR | O_CREAT | O_TRUNC)) < 0)
	{
		perror("failed to open input file\n");
		return 1;
	}

	if(ioctl(dev_fd, 0x12345677, ip) == -1)	//0x12345677 : connect to master in the device
	{
		perror("ioclt create slave socket error\n");
		return 1;
	}

    write(1, "ioctl success\n", 14);

	/*=======================added====================*/
	size_t disk_file_size = 0; PAGE_SIZE = getpagesize();
	/*=======================added====================*/


	switch(method[0])
	{
		case 'f'://fcntl : read()/write()
			do
			{
				ret = read(dev_fd, buf, sizeof(buf)); // read from the the device
				write(file_fd, buf, ret); //write to the input file
				file_size += ret;
			}while(ret > 0);
			break;
		case 'm'://mmap
			while(1){
				/*if(posix_fallocate(file_fd, disk_file_size, offset + PAGE_SIZE - disk_file_size) != 0){
					perror("Failed to fallocate: "); exit(1);
				}else{
					disk_file_size += (offset + PAGE_SIZE - disk_file_size);
					fprintf(stdout, "offset %d  file_size %d  disk_file_size %d\n", offset, file_size, disk_file_size); fflush(stdout);
				}
				file_address = (char *)mmap(NULL, PAGE_SIZE, PROT_WRITE|PROT_READ|PROT_EXEC, MAP_SHARED, file_fd, offset);
				if(file_address == MAP_FAILED){
					perror("Failed to mmap for write:"); exit(1);
				}
				if((ret = read(dev_fd, file_address, PAGE_SIZE)) <= 0){
					fprintf(stdout, "EOF!\n"); fflush(stdout); break;
				}
				write(STDOUT_FILENO, file_address, ret);
				fprintf(stdout, "read ret = %d\n", ret); fflush(stdout);
				file_size += ret;  offset = ret;
				if(munmap(file_address, PAGE_SIZE) == -1){
					perror("Failed to munmap: "); exit(1);
				}
				if(ret < PAGE_SIZE){
					fprintf(stdout, "ended!\n"); fflush(stdout); break;
				}
				*/
				
			}
			
	}

	if(ioctl(dev_fd, 0x12345679) == -1)// end receiving data, close the connection
	{
		perror("ioclt client exits error\n");
		return 1;
	}
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
	printf("Transmission time: %lf ms, File size: %d bytes\n", trans_time, file_size / 8);


	close(file_fd);
	close(dev_fd);
	return 0;
}

