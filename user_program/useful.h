#ifndef _USEFUL_H
#define _USEFUL_H
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <errno.h>

#define PRINT(ARGS...)  do{fprintf(stdout,ARGS);fflush(stdout);}while(0)
	
extern size_t PAGE_SIZE;

void perror_exit(char *string, int value){
	perror(string); exit(value);
}
#endif
