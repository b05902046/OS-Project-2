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

int main (int argc, char* argv[])
{
	char fBuf[512], mBuf[PAGE_SIZE];
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
	size_t disk_file_size = 0; size_t real_page_size = getpagesize();
	if(real_page_size != PAGE_SIZE){ PRINT("real page size %lu  !=  PAGE_SIZE %lu", real_page_size, PAGE_SIZE); exit(1);}
	
	/*=======================added====================*/

	#ifdef DEBUG
		PRINT("Initial file size %lu\n", file_size);
	#endif
	switch(method[0])
	{
		case 'f'://fcntl : read()/write()
			do
			{
				ret = read(dev_fd, fBuf, 512); // read from the the device
				#ifdef DEBUG
					PRINT("read ret = %lu\n", ret);
				#endif
				if(ret == -1) perror_exit("Failed to read: ", 1);
				write(file_fd, fBuf, ret); //write to the input file
				file_size += ret;
				#ifdef DEBUG
					PRINT("read %lu     file size = %lu\n", ret, file_size);
				#endif
			}while(ret > 0);
			break;
		/*case 'm'://mmap
			while(1){
				ret = read(dev_fd, buf, PAGE_SIZE);
				#ifdef DEBUG
					PRINT("read ret = %lu\n", ret);
				#endif
				if(ret == 0){
					#ifdef DEBUG
						PRINT("EOF!\n");
					#endif
					PRINT("read again ret = %lu\n", read(dev_fd, buf, PAGE_SIZE));
					break;
				}
				if(ret > PAGE_SIZE){ PRINT("ret %lu  >  PAGE_SIZE %lu\n", ret, PAGE_SIZE); exit(1);}
				mmap_write(&file_size, &disk_file_size, file_fd, buf, ret);
			}*/
			
	}

	if(ioctl(dev_fd, 0x12345679) == -1)// end receiving data, close the connection
	{
		perror("ioclt client exits error\n");
		return 1;
	}
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
	printf("Transmission time: %lf ms, File size: %lu bytes\n", trans_time, file_size / 8);


	close(file_fd);
	close(dev_fd);
	return 0;
}

