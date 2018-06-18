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


#define BUF_SIZE 512
size_t get_filesize(const char* filename);//get the size of the input file


int main (int argc, char* argv[])
{
	char fBuf[BUF_SIZE], *mBuf;
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
	size_t sent = 0, real_page_size = getpagesize();
	if(PAGE_SIZE != real_page_size){ PRINT("real_page_size %lu %lu != PAGE_SIZE\n", real_page_size, PAGE_SIZE); exit(1);}
	/*==============   added   ===================*/


	if(ioctl(dev_fd, 0x12345677) == -1) //0x12345677 : create socket and accept the connection from the slave
	{
		perror("ioclt server create socket error\n");
		return 1;
	}

	switch(method[0])
	{
		case 'f': //fcntl : read()/write()
			PRINT("Using FCNTL");
			write(STDOUT_FILENO, "Using FCNTL", 11);
			do
			{
				ret = read(file_fd, fBuf, BUF_SIZE); // read from the input file
				write(dev_fd, fBuf, ret);//write to the the device
			}while(ret > 0);
			break;
		case 'm': //mmap
			PRINT("Using MMAP\n");
			while(1){
				PRINT("In while\n");
				int sendTimes; size_t left = file_size - sent;
				if(left < PAGE_SIZE){
					sendTimes = (left + 511)/512;
					if(sendTimes > 0){
						mBuf = mmap_read(file_fd, sent, PAGE_SIZE);
						for(int i=0;i<sendTimes;++i){
							size_t num = (left < 512)? left:512;
							write(dev_fd, &mBuf[i*512], num);
							PRINT("write %lu\n", num);
							write(STDOUT_FILENO, &mBuf[i*512], num);
							left -= num; sent += num;
						}
					}
					break;
				}else{
					mBuf =mmap_read(file_fd, sent, PAGE_SIZE);
					for(int i=0;i<8;++i){
						write(dev_fd, &mBuf[i*512], 512);
						PRINT("write %lu\n", 512UL);
						write(STDOUT_FILENO, &mBuf[i*512], 512);
						left -= 512; sent += 512;
					}
				}
			}
	}
	read(dev_fd, fBuf, 3);
	write(STDOUT_FILENO, fBuf, 3);
	if(ioctl(dev_fd, 0x12345679) == -1) // end sending data, close the connection
	{
		perror("ioclt server exits error\n");
		return 1;
	}
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
	printf("Transmission time: %lf ms, File size: %lu bytes\n", trans_time, file_size / 8);

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
