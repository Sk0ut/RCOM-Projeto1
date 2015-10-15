#include "utils.h"
#include "linklayer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

static struct termios oldtio;

int llopen(int port, int flag){

    struct termios newtio;

	/* Open the serial port for sending the message */
    printf("Configuring port\n");

    if ( tcgetattr(port,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));

    /* 	Control, input, output flags */
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD; /*  */
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = OPOST;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */


    tcflush(port, TCIFLUSH);

    if ( tcsetattr(port,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    printf("Port configured\n");

	if(flag == TRANSMITTER)
		return llopen_transmitter(port);
	else if (flag == RECEIVER)
		return llopen_receiver(port);
	else
		return -1;
}

int llopen_transmitter(int port){
	char ua[MAX_STRING_SIZE];
	char buffer[] = {SERIAL_FLAG,
					SERIAL_A_COM_TRANSMITTER,
					SERIAL_C_SET,
					SERIAL_A_COM_TRANSMITTER^SERIAL_C_SET,
					SERIAL_FLAG};

	printf("Transmitter open sequence\n");
	printf("Sending SET\n");
	write(port,buffer,5);

	printf("Reading from port\n");
	int length = serial_read_string(port,ua);
	printf("Validating string\n");
	is_valid_string(ua,length);
	if(ua[C_FLAG_INDEX] == SERIAL_C_UA) {
		printf("Received UA\n"); 
		return 0;
	}
	else {
		printf("Different string from UA\n");
		return -1;
	}
}

int llopen_receiver(int port){
	char set[MAX_STRING_SIZE];
	printf("Receiver open sequence\n");
	printf("Reading from port\n");
	int length = serial_read_string(port,set);
	printf("Validating string\n");
	is_valid_string(set,length);
	if(set[C_FLAG_INDEX] == SERIAL_C_SET){
		printf("Received SET\n");
		char buffer[] = {SERIAL_FLAG,
					SERIAL_A_COM_TRANSMITTER,
					SERIAL_C_UA,
					SERIAL_A_COM_TRANSMITTER^SERIAL_C_UA,
					SERIAL_FLAG};
		printf("Sending UA\n");
		write(port,buffer,5);
		return 0;
	}
	else return -1;
}

int llclose(int fd){
	printf("Restoring port configurations\n");
	if (tcsetattr(fd,TCSANOW,&oldtio) == -1) {
	      perror("tcsetattr");
	      exit(-1);
    }
    close(fd);
}
