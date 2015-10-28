#include "linklayer.h"

#include <stdio.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <stdint.h>

#define BAUDRATE B38400
#define PACKAGE_DATA 0
#define PACKAGE_START 1
#define PACKAGE_END 2
#define PACKAGE_T_SIZE 0
#define PACKAGE_T_NAME 1

typedef struct {
	int fd;
	char name[255];
	uint32_t size;
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

void printUsage(int flag, char* argv0){
	if(flag == TRANSMITTER)
		printf("Usage: %s TRANSMITTER /dev/ttyS<portNo> <filepath> <flags>\n", argv0);
	else
		printf("Usage: %s RECEIVER /dev/ttyS<portNo> <flags>\n", argv0);
	printf("Flags: -b [BAUDRATE] - Sets the baudrate.\n");
	printf("       -t [TIMEOUT] - Sets the timeout before attempting to retransmit (default: 3).\n");
	printf("       -m [MAXTRIES] - Sets the maximum mumber of tries to transmit a message (default: 3). \n");
	printf("       -i [MAX_I_FRAME_SIZE] - Sets the maximum I frame size (before stuffing) (default: 255).\n");
}

int app_transmitter(int argc, char **argv) {

	int flag = TRANSMITTER;

	if(argc < 4){
		printUsage(flag, argv[0]);
		return 1;
	}

	File_info_t file_info;
	int port;
	int baudrate = BAUDRATE;
	int max_tries = 3;
	int timeout = 3;
	int max_frame_size = 255;
	char* filePath = argv[3];

	int arg;
	int changeMask[] = {FALSE, FALSE, FALSE, FALSE};
	for(arg = 4; arg < argc; ++arg){
		switch(argv[i]){
			case "-b":
				if(changeMask[0] == FALSE){
					changeMask[0] == TRUE;
					sscanf(argv[++arg], "%d", &baudrate);
					break;
				}
				else {
					printf("Baudrate defined more than once. First defined as value: %d\n", baudrate);
					return 1;
				}
			case "-t":			
				if(changeMask[1] == FALSE){
					changeMask[1] == TRUE;
					sscanf(argv[++arg], "%d", &timeout);
					break;
				}
				else {
					printf("Timeout defined more than once. First defined as value: %d\n", timeout);
					return 1;
				}
			case "-m":
				if(changeMask[2] == FALSE){
					changeMask[2] == TRUE;
					sscanf(argv[++arg], "%d", &max_tries);
					break;
				}
				else {
					printf("Maximum retransmission tries cap defined more than once. First defined as value %d\n", max_tries);
					return 1;
				}
			case "-i":
				if(changeMask[3] == FALSE){
					changeMask[3] == TRUE;
					sscanf(argv[++arg], "%d", &max_frame_size);
					break;
				}
				else {
					printf("Maximum I frame size defined more than once. First defined as value %d\n", max_frame_size);
					return 1;
				}
			default:

		}

	}

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
	printf("Fd: %d, Name: %s, Size:%d\n", file_info.fd, file_info.name, file_info.size);

	if (llopen(link_layer) != 0)
		return 1;
	
	unsigned int segmentSize = get_max_message_size(link_layer) - 4;
	printf("Segment size: %d\n",segmentSize);

	unsigned char segment[segmentSize];

	// Build Control package
	segment[0] = PACKAGE_START;
	segment[1] = PACKAGE_T_NAME;
	uint8_t file_name_size = strlen(file_info.name) + 1;
	printf("file name size: %d\n", file_name_size);
	segment[2] = file_name_size;
	memcpy(&(segment[3]), file_info.name, file_name_size);
	printf("File name: %s\n", &(segment[3]));
	segment[3+file_name_size] = PACKAGE_T_SIZE;
	segment[4+file_name_size] = sizeof(uint32_t);
	*((uint32_t *)&segment[5+file_name_size]) = file_info.size;
	printf("File size: %d\n", *((int *)&(segment[5+file_name_size])));
	
	int i;
	printf("Control package: ");
	for(i= 0; i < file_name_size+9;i++){
		printf("0x%x ",segment[i]);
	}
	printf("\n");

	//Send start
	if (llwrite(link_layer, segment, file_name_size + 9) != file_name_size + 9) {
		printf("Error sending start control package\n");
		return 1;
	}

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
			
			if(llwrite(link_layer,segment, length+4) != length + 4){
				printf("Error in sending data package");
				return 1;
			}
			
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
	segment[4+file_name_size] = sizeof(uint32_t);
	*((uint32_t *)&segment[5+file_name_size]) = file_info.size;


	
	if (llwrite(link_layer, segment, file_name_size + 9) != file_name_size + 9) {
		printf("Error starting end control packages\n");
		return 1;
	}
	
	
	if (llclose(link_layer) != 0)
		return 1;
	
	lldelete(link_layer);

	return 0;
}

int app_receiver(int argc, char **argv) {

	int flag = RECEIVER;

	if (argc < 3) {
		printUsage(flag, argv[0]);
		return 1;
	}

	int port;
	int baudrate = BAUDRATE;
	int max_tries = 3;
	int timeout = 3;
	int max_frame_size = 255;

	int arg;
	int changeMask[] = {FALSE, FALSE, FALSE, FALSE};
	for(arg = 4; arg < argc; ++arg){
		switch(argv[i]){
			case "-b":
				if(changeMask[0] == FALSE){
					changeMask[0] == TRUE;
					sscanf(argv[++arg], "%d", &baudrate);
					break;
				}
				else {
					printf("Baudrate defined more than once. First defined as value: %d\n", baudrate);
					return 1;
				}
			case "-t":			
				if(changeMask[1] == FALSE){
					changeMask[1] == TRUE;
					sscanf(argv[++arg], "%d", &timeout);
					break;
				}
				else {
					printf("Timeout defined more than once. First defined as value: %d\n", timeout);
					return 1;
				}
			case "-m":
				if(changeMask[2] == FALSE){
					changeMask[2] == TRUE;
					sscanf(argv[++arg], "%d", &max_tries);
					break;
				}
				else {
					printf("Maximum retransmission tries cap defined more than once. First defined as value %d\n", max_tries);
					return 1;
				}
			case "-i":
				if(changeMask[3] == FALSE){
					changeMask[3] == TRUE;
					sscanf(argv[++arg], "%d", &max_frame_size);
					break;
				}
				else {
					printf("Maximum I frame size defined more than once. First defined as value %d\n", max_frame_size);
					return 1;
				}
			default:

		}

	}

	LinkLayer link_layer = llinit(port, flag, baudrate, max_tries, timeout, max_frame_size);
	printf("port: %d\n", port);
	printf("flag: %d\n", flag);
	printf("baudrate: %d\n", baudrate);
	printf("max_tries: %d\n", max_tries);
	printf("timeout: %d\n", timeout);
	printf("max_frame_size: %d\n", max_frame_size);
	
	if (llopen(link_layer) != 0)
		return 1;
	
	// Read start
	unsigned int maxSegmentLength = get_max_message_size(link_layer);
	char segment[maxSegmentLength];
	int segmentLength = llread(link_layer, segment);
	
	if (segmentLength <= 0) {
		printf("Error llread");
		return 1;
	}
	
	int i;
	printf("Read:");
	for (i = 0; i < segmentLength; ++i)
		printf(" 0x%.2x", segment[i]);
	printf("\n");
	
	File_info_t file_info;
	
	if(segment[0] == PACKAGE_START){
		i=1;
		while(i < segmentLength){
			char type = segment[i];
			unsigned char size = segment[i+1];
			switch(type){
				case PACKAGE_T_SIZE:
					file_info.size = *((uint32_t *) &segment[i+2]);
					printf("File info size: %d", file_info.size);
					break;
				case PACKAGE_T_NAME:
					memcpy(file_info.name,&segment[i+2],size);
					printf("File info name: %s", file_info.name);
					break;
			}
			i += 2 + size;
		}
	}

	// Create file
	file_info.fd = creat(file_info.name, 0666);
	while (1) {
		segmentLength = llread(link_layer, segment);
	
		if (segmentLength <= 0) {
			printf("Error llread");
			return 1;
		}		
		printf("Read:");
		for (i = 0; i < segmentLength; ++i)
			printf(" 0x%.2x", segment[i]);
		printf("\n");
	
		//check end conditions
		if (segment[0] == PACKAGE_END)
			break;
		
		// copy to file
		write(file_info.fd, &(segment[4]), segmentLength - 4);
	}

	if (close(file_info.fd) != 0)
		return 1;
	
	if (llclose(link_layer) != 0)
		return 1;
	
	lldelete(link_layer);
	
	return 0;
}

int main(int argc, char** argv){

	if (argc == 1)
	{
		printf("Usage: %s RECEIVER /dev/ttyS<portNr> <baudrate> <max_tries> <timeout> <max_frame_size>\n", argv[0]);
		printf("Usage: %s TRANSMITTER /dev/ttyS<portNr> <filepath> <baudrate> <max_tries> <timeout> <max_frame_size>\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "TRANSMITTER") == 0)
		return app_transmitter(argc, argv);
	else if (strcmp(argv[1], "RECEIVER") == 0)
		return app_receiver(argc, argv);
	else
	{
		printf("Usage: %s RECEIVER /dev/ttyS<portNr> <baudrate> <max_tries> <timeout> <max_frame_size>\n", argv[0]);
		printf("Usage: %s TRANSMITTER /dev/ttyS<portNr> <filepath> <baudrate> <max_tries> <timeout> <max_frame_size>\n", argv[0]);
		return 1;
	}
}