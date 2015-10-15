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
#define MODEMDEVICE "/dev/ttyS"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

static struct termios oldtio;

int llopen(int port, int flag){

    struct termios newtio;

    char serial_name[strlen(MODEMDEVICE) + 2 + 1];
    sprintf(serial_name, "%s%d", MODEMDEVICE, port);

    int fd = open(serial_name, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(serial_name); return -1; }

	/* Open the serial port for sending the message */

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      return -1;
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


    tcflush(fd, TCIFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

	if(flag == TRANSMITTER)
		return llopen_transmitter(fd);
	else if (flag == RECEIVER)
		return llopen_receiver(fd);
	else
		return -1;
}

int llopen_transmitter(int fd){
	char ua[MAX_STRING_SIZE];
	char buffer[] = {SERIAL_FLAG,
					SERIAL_A_COM_TRANSMITTER,
					SERIAL_C_SET,
					SERIAL_A_COM_TRANSMITTER^SERIAL_C_SET,
					SERIAL_FLAG};
	write(fd,buffer,5);
	int length = serial_read_string(fd,ua);
	is_valid_string(ua,length);
	if(ua[C_FLAG_INDEX] == SERIAL_C_UA) 
		return fd;
	else
		return -1;
}

int llopen_receiver(int fd){
	char set[MAX_STRING_SIZE];
	int length = serial_read_string(fd,set);
	is_valid_string(set,length);
	if(set[C_FLAG_INDEX] == SERIAL_C_SET){
		char buffer[] = {SERIAL_FLAG,
					SERIAL_A_COM_TRANSMITTER,
					SERIAL_C_UA,
					SERIAL_A_COM_TRANSMITTER^SERIAL_C_UA,
					SERIAL_FLAG};
		write(fd,buffer,5);
		return fd;
	}
	else return -1;
}

int llclose(int fd){
	if (tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);
}