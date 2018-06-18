#ifndef _USEFUL_H
#define _USEFUL_H
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <errno.h>

#define PAGE_SIZE 4096UL
#define PRINT(ARGS...)  do{fprintf(stdout,ARGS);fflush(stdout);}while(0)

void perror_exit(char *string, int value){
	perror(string); exit(value);
}
#endif
