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
	char fBuf[512], *mBuf, pageBuf[PAGE_SIZE];
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
	size_t disk_size = 0, read_accu = 0; size_t real_page_size = getpagesize(); int flag;
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
					PRINT("read ret = %lu\n", ret);
				if(ret == -1) perror_exit("Failed to read: ", 1);
				write(file_fd, fBuf, ret); //write to the input file
				file_size += ret;
					PRINT("read %lu     file size = %lu\n", ret, file_size);
				}while(ret > 0);
			break;
		case 'm'://mmap
			flag = 1;
			while(flag){
				mBuf = prepare_write_buffer(file_size, &disk_size, file_fd);
				read_accu = 0;
				for(int i=0;i<8;++i){
					ret = read(dev_fd, &pageBuf[i*512], 512);
					if(ret == -1){
						perror("Failed to read: "); flag = 0; break;
					}else if(ret == 0){
						PRINT("EOF!\n"); flag = 0; break;
					}else{
						read_accu += ret; PRINT("read_accu = %lu\n", read_accu);
					}
				}
				if(read_accu > 0){
					//write(STDOUT_FILENO, pageBuf, read_accu);
					memcpy(mBuf, pageBuf, read_accu);
					//PRINT("memcpy over\n");
					file_size += read_accu;
				}
				PRINT("file size = %lu   disk size = %lu\n", file_size, disk_size);
				munmap_write_buffer(mBuf);
				if(read_accu != PAGE_SIZE) break;
			}
			
	}
	write(dev_fd, "EOF", 3);
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

