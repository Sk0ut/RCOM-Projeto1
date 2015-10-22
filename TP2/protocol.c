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
#include <signal.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

static struct termios oldtio;
static int tries;

void sig_alarm_handler(int sig) {
    if (sig == SIGALRM) {
        printf("Alarm!\n");
        ++tries;
    }
}

int llopen(int port, int flag){

    struct termios newtio;

    char serial_name[strlen(MODEMDEVICE) + 2 + 1];
    sprintf(serial_name, "%s%d", MODEMDEVICE, port);

    int fd = open(serial_name, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(serial_name); return -1; }

	/* Open the serial port for sending the message */
    printf("Configuring port\n");

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
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */


    tcflush(fd, TCIFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    printf("Port configured\n");
    
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = sig_alarm_handler;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("Error: cannot handle SIGALRM");
        return -1;
    }
    
    //signal(SIGALRM, sig_alarm_handler);

	if(flag == TRANSMITTER)
		return llopen_transmitter(fd);
	else if (flag == RECEIVER)
		return llopen_receiver(fd);
	else
		return -1;
}

int ua_validator (char* buffer, int length){
    return (length == 3) && (buffer[C_FLAG_INDEX] == SERIAL_C_UA);
}

int set_validator (char* buffer, int length){
    return (length == 3) && (buffer[C_FLAG_INDEX] == SERIAL_C_SET);
}

int disc_validator (char* buffer, int length){
    return (length == 3) && (buffer[C_FLAG_INDEX] == SERIAL_C_DISC);
}

int llopen_transmitter(int fd){
	char buf[MAX_STRING_SIZE];
	char buffer[] = {SERIAL_FLAG,
			SERIAL_A_COM_TRANSMITTER,
			SERIAL_C_SET,
			SERIAL_A_COM_TRANSMITTER^SERIAL_C_SET,
			SERIAL_FLAG};


	printf("Transmitter open sequence\n");

    int length;
    tries = 0;
    while (tries < 3) {
        printf("Sending SET\n");
    	write(fd,buffer,5);
		alarm(3);
		while (1) {
            length = serial_read_string(fd,buf);
			if (length == -1)
				break;

			printf("Validating string\n");
			if(is_valid_string(buf,length) && ua_validator(buf, length)) {
                printf("Valid string\n"); 
                alarm(0);
                break;
	   		}
        }
        if (length != -1)
            break;
    }
    if (tries == 3)    
        return -1;

    return fd;
}

int llopen_receiver(int fd){
	char buf[MAX_STRING_SIZE];
	printf("Receiver open sequence\n");
	printf("Reading from port\n");

    char ua[] = {SERIAL_FLAG,
		    	SERIAL_A_ANS_RECEIVER,
			    SERIAL_C_UA,
			    SERIAL_A_ANS_RECEIVER^SERIAL_C_UA,
			    SERIAL_FLAG};

    int length;
    while (1) {
        length = serial_read_string(fd,buf);
        if (length <= 0)
            continue;        
        printf("Validating string\n");
	    if(is_valid_string(buf,length) && set_validator(buf, length))
           break;
    }

    write(fd, ua, 5);
    return fd;
}

int llclose_transmitter(int fd){
	char buf[MAX_STRING_SIZE];
    char disc[] = {SERIAL_FLAG,
		    	SERIAL_A_ANS_RECEIVER,
			    SERIAL_C_DISC,
			    SERIAL_A_ANS_RECEIVER^SERIAL_C_DISC,
			    SERIAL_FLAG};

    char ua[] = {SERIAL_FLAG,
		    	SERIAL_A_ANS_RECEIVER,
			    SERIAL_C_UA,
			    SERIAL_A_ANS_RECEIVER^SERIAL_C_UA,
			    SERIAL_FLAG};
  
    int length;
    tries = 0;
    while (tries < 3) {
        printf("Sending DISC\n");
    	write(fd,disc,5);
		alarm(3);
		while (1) {
            length = serial_read_string(fd,buf);
			if (length == -1)
				break;

			printf("Validating string\n");
			if(is_valid_string(buf,length) && disc_validator(buf, length)) {
                printf("Valid string\n"); 
                alarm(0);
                break;
	   		}
        }
        if (length != -1)
            break;
    }

    if (tries == 3)    
        return -1;

    write(fd, ua, 5);
    return fd;
}

int llclose_receiver(int fd){
	char buf[MAX_STRING_SIZE];
	char disc[] = {SERIAL_FLAG,
		    	SERIAL_A_ANS_RECEIVER,
			    SERIAL_C_DISC,
			    SERIAL_A_ANS_RECEIVER^SERIAL_C_DISC,
			    SERIAL_FLAG};

	/* Recepcao do DISC */

	//TODO: Fazer uma funcao deste ciclo
	int length;
	while (1) {
        length = serial_read_string(fd,buf);
        if (length <= 0)
            continue;        
        printf("Validating string\n");
	    if(is_valid_string(buf,length) && disc_validator(buf, length))
           break;
    }

    /* Envio do DISC */
    write(fd,disc,5);

    /* Recepcao do UA */
    /* Nota:  infinto àn Debater com a professora ciclo infinto à espera do UA*/
    while (1) {
        length = serial_read_string(fd,buf);
        if (length <= 0)
            continue;        
        printf("Validating string\n");
	    if(is_valid_string(buf,length) && ua_validator(buf, length))
           break;
    }

	return fd;
}

int llclose(int fd, int flag){
	if(flag == TRANSMITTER)
		llclose_transmitter(fd);
	else if (flag == RECEIVER)
		llclose_receiver(fd);
	else
		return -1;
	printf("Restoring port configurations\n");
	if (tcsetattr(fd,TCSANOW,&oldtio) == -1) {
	      perror("tcsetattr");
	      exit(-1);
    }
    close(fd);
}
