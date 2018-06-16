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
#include "forslave.h"

size_t PAGE_SIZE;
int main (int argc, char* argv[])
{
	char *buf;
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size = 0, content_len;
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
	if((buf = (char *)malloc(PAGE_SIZE) == NULL)) perror_exit("Failed to malloc buf: ", 1);
	
	/*=======================added====================*/


	switch(method[0])
	{
		case 'f'://fcntl : read()/write()
			#ifdef DEBUG
				PRINT("Initial file size = %u\n", file_size);
			#endif
			while(1){
				read(dev_fd, &ret, sizeof(size_t));
				#ifdef DEBUG
					if(ret == 0){ PRINT("EOF\n"); break;}
					else if(ret == 4294967295U) exit(1);
					else PRINT("content_len = %u\n", ret);
				#endif
				if(ret == 0) break;
				read(dev_fd, buf, ret); // read from the the device
				write(file_fd, buf, ret); //write to the input file
				file_size += ret;
				#ifdef DEBUG
					PRINT("file_size %u after write %u\n", file_size, ret);
				#endif
			}
			break;
		case 'm'://mmap
			while(1){
				#ifdef NO_KSOCKET_MMAP
					if((ret = read(dev_fd, &content_len, sizeof(size_t))) != sizeof(size_t)){
						#ifdef DEBUG
							PRINT("size_t %u != ret = %u   content_len = %u\n", sizeof(size_t), ret, content_len);
						#endif
						//perror_exit("Failed to read content_len: ", 1);						
					}
					#ifdef DEBUG
						PRINT("content_len = %u\n", content_len);
					#endif
					if((ret = read(dev_fd, buf, content_len)) != content_len){
						PRINT("Weird read content: ret = %u\n", ret); perror_exit("Failed to read?: ", 1);
					}
					file_address = buf;
				#else
					ret = mmap_read(dev_fd, &file_address);
				#endif
				if(ret == 0) break;
				mmap_write(&file_size, &disk_file_size, file_fd, file_address, ret);
				munmap_for_read(file_address, ret);
			}
			
	}

	if(ioctl(dev_fd, 0x12345679) == -1)// end receiving data, close the connection
	{
		perror("ioclt client exits error\n");
		return 1;
	}
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
	printf("Transmission time: %lf ms, File size: %u bytes\n", trans_time, file_size / 8);


	close(file_fd);
	close(dev_fd);
	return 0;
}

