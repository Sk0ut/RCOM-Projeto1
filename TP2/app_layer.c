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

int app_transmitter(int argc, char **argv) {
	if(argc != 8){
		printf("Usage: %s <TRANSMITTERorRECEIVER> <port> <filepath> <baudrate> <max_tries> <timeout> <max_frame_size>\n", argv[0]);
		return 1;
	}

	int flag = TRANSMITTER;
	File_info_t file_info;
	int port;
	int baudrate = BAUDRATE;
	int max_tries = 3;
	int timeout = 3;
	int max_frame_size = 255;
	char* filePath = argv[3];

	sscanf(argv[2],"/dev/ttyS%d",&port);
	sscanf(argv[5], "%d", &max_tries);
	sscanf(argv[6], "%d", &timeout);
	sscanf(argv[7], "%d", &max_frame_size);

	LinkLayer link_layer = llinit(port, flag, baudrate, max_tries, timeout, max_frame_size);
	printf("port: %d\n", port);
	printf("flag: %d\n", flag);
	printf("baudrate: %d\n", baudrate);
	printf("max_tries: %d\n", max_tries);
	printf("timeout: %d\n", timeout);
	printf("max_frame_size: %d\n", max_frame_size);
	printf("filepath: %s\n", filePath);
	
	
	if(get_file_info(&file_info, filePath) == -1)
		return 1;
	printf("Fd: %d, Name: %s, Size:%ld\n", file_info.fd, file_info.name, file_info.size);

	unsigned int segmentSize = get_max_message_size(link_layer) - 4;
	printf("Segment size: %d\n",segmentSize);

	unsigned char segment[segmentSize];

	// Build Control package
	segment[0] = PACKAGE_START;
	segment[1] = PACKAGE_T_NAME;
	unsigned char file_name_size = strlen(file_info.name) + 1;
	segment[2] = file_name_size;
	memcpy(&(segment[3]), file_info.name, file_name_size);
	segment[3+file_name_size] = PACKAGE_T_SIZE;
	segment[4+file_name_size] = 2;
	segment[5+file_name_size] = (file_info.size & 0xFF00) >> 8;
	segment[6+file_name_size] = (file_info.size & 0xFF);

	int i;

	printf("Control package: ");
	for(i= 0; i < file_name_size+7;i++){
		printf("0x%x ",segment[i]);
	}
	printf("\n");
	//Send start
	//Send data
	int length;
	unsigned char sequenceNumber = 0;
	printf("Data packages:\n");
	do {
		length = read(file_info.fd, &(segment[4]), segmentSize);
		
		if (length > 0) {
			segment[0] = PACKAGE_DATA;
			segment[1] = sequenceNumber;
			segment[2] = (length & 0xFF00) >> 8;
			segment[3] = length & 0xFF;
			// send segment;
			int j;
			for (j = 0; j < length + 4; ++j)
				printf("0x%.2x ", segment[j]);
			printf("\n");
			++sequenceNumber;
		}
	} while (length > 0);
	
		
	//Send end
	segment[0] = PACKAGE_END;
	segment[1] = PACKAGE_T_NAME;
	segment[2] = file_name_size;
	memcpy(&(segment[3]), file_info.name, file_name_size);
	segment[3+file_name_size] = PACKAGE_T_SIZE;
	segment[4+file_name_size] = sizeof(long);
	*((long *)&(segment[5+file_name_size])) = file_info.size;
	
	lldelete(link_layer);

	return 0;
}

int app_receiver(int argc, char **argv) {
	// Read start
	// Create file
	// Read data
	// Find end

	return 0;
}

int main(int argc, char** argv){

	if (argc == 1)
		return 1;
	
	if (strcmp(argv[1], "TRANSMITTER") == 0)
		return app_transmitter(argc, argv);
	else if (strcmp(argv[1], "RECEIVER") == 0)
		return app_receiver(argc, argv);
	else
		return 1;
}