#include "linklayer.h"


typedef struct {
	int fd;
	char name[255];
	long size;
} File_info_t;


int main(int argc, char** argv){

	int flag;
	File_info_t file_info;
	char* filePath = argv[1];

	//Linklayer link_layer = llinit();

	if (flag == TRANSMITTER){
		if(get_info_file(&file_info, filePath) == -1)
			return 1;
		printf("Fd: %d, Name: %s, Size:%d\n", file_info.fd, file_info.name, file_info.size);
	}

	//lldelete(link_layer);

	return 0;
}

int file_info(File_info_t* file_info, char* filePath){
	int fd = open(filePath,O_RDONLY | O_BINARY);

	if(fd == -1)
		return -1;

	struct stat st;
	if(fstat(fd,&st) == -1)
		return -1;

	file_info->fd = fd;
	file_info->name = basename(filePath);
	file_info->size = st.st_size;

	return 0;
}