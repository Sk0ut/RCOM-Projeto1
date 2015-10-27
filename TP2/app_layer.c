#include "linklayer.h"

#include <stdio.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>

#define BAUDRATE B38400
#define PACKAGE_DATA 0
#define PACKAGE_START 1
#define PACKAGE_END 2
#define PACKAGE_T_SIZE 0
#define PACKAGE_T_NAME 1

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

	if(argc != 7){
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


	LinkLayer link_layer = llinit(port, flag, baudrate, max_tries, timeout, max_frame_size);

	if (flag == TRANSMITTER){
		if(get_file_info(&file_info, filePath) == -1)
			return 1;
		printf("Fd: %d, Name: %s, Size:%ld\n", file_info.fd, file_info.name, file_info.size);
	}

	unsigned int segmentSize = get_max_message_size(link_layer) - 4;
	printf("Segment size: %d\n",segmentSize);

	char segment[segmentSize];

	if (flag == TRANSMITTER) {
		// Build Control package
		segment[0] = PACKAGE_START;
		segment[1] = PACKAGE_T_NAME;
		unsigned char file_name_size = strlen(file_info.name) + 1;
		segment[2] = file_name_size;
		memcpy(&(segment[3]), file_info.name, file_name_size);
		segment[3+file_name_size] = PACKAGE_T_SIZE;
		segment[4+file_name_size] = sizeof(long);
		*((long *)&(segment[5+file_name_size])) = file_info.size;

		int i;

		for(i= 0; i < 5+file_name_size+sizeof(long);i++){
			printf("0x%x ",segment[i]);
		}
		printf("\n");
		//Send start
		//Send data
		//Send end
		segment[0] = PACKAGE_END;
		segment[1] = PACKAGE_T_NAME;
		segment[2] = file_name_size;
		memcpy(&(segment[3]), file_info.name, file_name_size);
		segment[3+file_name_size] = PACKAGE_T_SIZE;
		segment[4+file_name_size] = sizeof(long);
		*((long *)&(segment[5+file_name_size])) = file_info.size;
		
	} else if (flag == RECEIVER) {
		// Read start
		// Create file
		// Read data
		// Find end
	}

	lldelete(link_layer);

	return 0;
}