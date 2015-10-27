#include "linklayer.h"

#include <stdio.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>

#define BAUDRATE B38400

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

/*
	./nserial /dev/ttyS0 ./file.txt baudrate max_tries timeout max_frame_size
*/

int main(int argc, char** argv){

	if(argc != 6){
		printf("Usage: %s <port> <filepath> <baudrate> <max_tries> <timeout> <max_frame_size>\n", argv[0]);
		return 1;
	}

	int flag = TRANSMITTER;
	File_info_t file_info;
	int port;
	int baudrate = BAUDRATE;
	int max_tries = 3;
	int timeout = 3;
	int max_frame_size = 255;
	char* filePath = argv[2];

	sscanf(argv[1],"/dev/ttyS%d",&port);
	sscanf(argv[4], "%d", &max_tries);
	sscanf(argv[5], "%d", &timeout);
	sscanf(argv[6], "%d", &max_frame_size);


	Linklayer link_layer = llinit(port, flag, max_tries, timeout, max_frame_size);

	if (flag == TRANSMITTER){
		if(get_file_info(&file_info, filePath) == -1)
			return 1;
		printf("Fd: %d, Name: %s, Size:%ld\n", file_info.fd, file_info.name, file_info.size);
	}



	lldelete(link_layer);

	return 0;
}