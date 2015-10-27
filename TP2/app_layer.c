#include "linklayer.h"

#include <stdio.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


typedef struct {
	int fd;
	char name[255];
	long size;
} File_info_t;


int get_file_info(File_info_t* file_info, char* filePath){
	int fd = open(filePath,O_RDONLY);

	if(fd == -1)
		return -1;

	struct stat st;
	if(fstat(fd,&st) == -1)
		return -1;

	file_info->fd = fd;
	sprintf(file_info->name, "%s", basename(filePath));
	file_info->size = st.st_size;

	return 0;
}

int main(int argc, char** argv){

	int flag = TRANSMITTER;
	File_info_t file_info;
	char* filePath = argv[1];

	//Linklayer link_layer = llinit();

	if (flag == TRANSMITTER){
		if(get_file_info(&file_info, filePath) == -1)
			return 1;
		printf("Fd: %d, Name: %s, Size:%ld\n", file_info.fd, file_info.name, file_info.size);
	}

	//lldelete(link_layer);

	return 0;
}