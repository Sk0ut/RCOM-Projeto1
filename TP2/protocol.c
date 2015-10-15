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

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

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
	write(port,buffer,5);
	int length = serial_read_string(port,ua);
	is_valid_string(ua,length);
	if(ua[C_FLAG_INDEX] == SERIAL_C_UA) 
		return 0;
	else
		return -1;
}

int llopen_receiver(int port){
	char set[MAX_STRING_SIZE];
	int length = serial_read_string(port,set);
	is_valid_string(set,length);
	if(set[C_FLAG_INDEX] == SERIAL_C_SET){
		char buffer[] = {SERIAL_FLAG,
					SERIAL_A_COM_TRANSMITTER,
					SERIAL_C_UA,
					SERIAL_A_COM_TRANSMITTER^SERIAL_C_UA,
					SERIAL_FLAG};
		write(port,buffer,5);
		return 0;
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