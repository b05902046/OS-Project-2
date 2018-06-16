#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include "formaster.h"

size_t PAGE_SIZE;
size_t get_filesize(const char* filename);//get the size of the input file


int main (int argc, char* argv[])
{
	char *buf;
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size, offset = 0, tmp;
	char file_name[50], method[20];
	char *kernel_address = NULL, *file_address = NULL;
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	
	strcpy(file_name, argv[1]);
	strcpy(method, argv[2]);


	if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
	{
		perror("failed to open /dev/master_device\n");
		return 1;
	}
	gettimeofday(&start ,NULL);
	if( (file_fd = open (file_name, O_RDWR)) < 0 )
	{
		perror("failed to open input file\n");
		return 1;
	}

	if( (file_size = get_filesize(file_name)) < 0)
	{
		perror("failed to get filesize\n");
		return 1;
	}

	/*==============   added   ===================*/
	size_t sent;
	PAGE_SIZE = getpagesize();
	if((buf = (char *)malloc(PAGE_SIZE)) == NULL) perror_exit("Failed to malloc buf: ", 1);
	/*==============   added   ===================*/


	if(ioctl(dev_fd, 0x12345677) == -1) //0x12345677 : create socket and accept the connection from the slave
	{
		perror("ioclt server create socket error\n");
		return 1;
	}

	switch(method[0])
	{
		case 'f': //fcntl : read()/write()
			write(STDOUT_FILENO, "Using FCNTL", 11);
			do
			{
				ret = read(file_fd, buf, PAGE_SIZE); // read from the input file
				#ifdef DEBUG
					PRINT("send_len = %u\n", ret);
				#endif
				write(dev_fd, &ret, sizeof(size_t));
				write(dev_fd, buf, ret);//write to the the device
			}while(ret > 0);
			break;
		case 'm': //mmap
			while(offset < file_size){
				#ifdef DEBUG
					fprintf(stdout, "file_size %u\n", file_size); fflush(stdout);
				#endif
				sent = file_size - offset;
				if(sent > PAGE_SIZE) sent = PAGE_SIZE;
				file_address = mmap_read(file_fd, &offset, sent);
				#ifdef NO_KSOCKET_MMAP
					PRINT("NO_KSCOCKET_MMAP\n");
					write(dev_fd, &sent, sizeof(size_t));
					write(dev_fd, file_address, sent);
				#else
					mmap_write(dev_fd, file_address, sent);
				#endif
				munmap_for_read(file_address, sent);
			}
			sent = 0;
			#ifdef NO_KSOCKET_MMAP
				write(dev_fd, &sent, sizeof(size_t));
			#else
				mmap_write(dev_fd, &sent, sizeof(size_t));
			#endif
	}

	if(ioctl(dev_fd, 0x12345679) == -1) // end sending data, close the connection
	{
		perror("ioclt server exits error\n");
		return 1;
	}
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
	printf("Transmission time: %lf ms, File size: %u bytes\n", trans_time, file_size / 8);

	close(file_fd);
	close(dev_fd);

	return 0;
}

size_t get_filesize(const char* filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}
