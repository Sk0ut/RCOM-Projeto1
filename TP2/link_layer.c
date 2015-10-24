#include "linklayer.h"
#include "utils.h"

#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

static int tries;

int is_valid_s_u(const char* string);
int is_a_flag(const char a);
int is_c_flag(const char c);
int is_valid_bcc(const char a, const char c, const char bcc);
int is_valid_combination(const char a, const char c);
int is_valid_i(const char* string, int string_length);

struct LinkLayer_t {
	int fd;
	unsigned int baudrate;
	unsigned int timeout;
	unsigned int max_tries;
	unsigned int max_frame_size;
	int flag; // TRANSMITTER or RECEIVER
	struct termios oldtio;
	unsigned int sequence_number;
	char* buffer;
};

/* Validates string */
int is_valid_string(const char* string, const int string_length){
	if(string_length == 3){
		return is_valid_s_u(string);
	}
	else {
		return is_valid_i(string, string_length);
	}
}

/* Validates su flag*/
int is_valid_s_u(const char* string){
	if(!is_a_flag(string[A_FLAG_INDEX])){
		return FALSE;
	}

	if(!is_c_flag(string[C_FLAG_INDEX])){
		return FALSE;
	}

	if(!is_valid_bcc(string[A_FLAG_INDEX],string[C_FLAG_INDEX], string[BCC_FLAG_INDEX])){
		return FALSE;
	}

	return is_valid_combination(string[A_FLAG_INDEX], string[C_FLAG_INDEX]);
}

/* Validates a flag */
int is_a_flag(const char a){
	switch(a){
		case SERIAL_A_COM_TRANSMITTER:
		case SERIAL_A_COM_RECEIVER:
			return TRUE;
		default:
			return FALSE;
	}
}

/* Validates c flag */
int is_c_flag(const char c){
	switch(c){
		case SERIAL_C_SET:
		case SERIAL_C_DISC:
		case SERIAL_C_UA:
			return TRUE;
		default:
			return (c|C_FLAG_R_VALUE)==SERIAL_C_RR_N0 || (c|C_FLAG_R_VALUE)==SERIAL_C_RR_N1 ||
					(c|C_FLAG_R_VALUE)==SERIAL_C_REJ;
	}
}

/* Validates BCC Flag */
int is_valid_bcc(const char a, const char c, const char bcc){
	return (a^c) == bcc;
}

/* Validates the string */
int is_valid_combination(const char a, const char c){
	return TRUE;
}

/* Validates I String*/
/* TODO: Perguntar ao Flavinho se ele nao se importa de adicionar aqui um argumento para validar se e um i0/i1 correto tambem,
ou se prefere fazer de outra forma */
int is_valid_i(const char* string, int string_length){
	if(string_length < 5) //Pressupoe que e mandado pelo menos 1 byte de data
		return FALSE;

	if(!is_a_flag(string[A_FLAG_INDEX])){
		return FALSE;
	}

	if(!is_c_flag(string[C_FLAG_INDEX])){
		return FALSE;
	}

	if(!is_valid_bcc(string[A_FLAG_INDEX],string[C_FLAG_INDEX], string[BCC_FLAG_INDEX])){
		return FALSE;
	}

	/* TODO: Validacao de data */
	/* TODO: Bcc2, perguntar a prof como fazer*/

	return TRUE;
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

int rr_validator (char* buffer, int length, int type){
    switch(type){
        case RR0:
        return (length == 3) && (buffer[C_FLAG_INDEX] == SERIAL_C_RR_N0);
        case RR1:
        return (length == 3) && (buffer[C_FLAG_INDEX] == SERIAL_C_RR_N1);
        default:
        return -1;
    }

}

void sig_alarm_handler(int sig) {
    if (sig == SIGALRM) {
        printf("Alarm!\n");
        ++tries;
    }
}

void reset_alarm() {
	tries = 0;
}

int read_frame(LinkLayer link_layer) {
	int length = 0;
	char c;
	int res;

	do {
        printf("Reading from port\n");
		res = read(link_layer->fd, &c, 1);
		if (res < 1)
			return -1;
	} while(c != SERIAL_FLAG);

	while (length < 3) {	
		length = 0;

		do {
	        res = read(link_layer->fd, &c, 1);
			if (res < 1)
				return -1;
			if (c == SERIAL_FLAG)
				break;

			switch(c){
	            case SERIAL_ESCAPE:
	            	res = read(link_layer->fd, &c ,1);
	            	if (res < 1)
	            		return -1; 
	                if(c == SERIAL_FLAG_REPLACE)
	                    link_layer->buffer[length] = SERIAL_FLAG;
	                else if(c == SERIAL_ESCAPE_REPLACE)
	                    link_layer->buffer[length] = SERIAL_ESCAPE;
	                else
	                	return -1;
	            	break;
	            default:
	                link_layer->buffer[length] = c;
	                break;
        	}
			++length;
		} while(length < link_layer->max_frame_size);

		if (length == link_layer->max_frame_size)
			return -1;
	}

	int i;
	printf("Values read: ");
	for (i = 0; i < length; ++i)
		printf("0x%.2x ", link_layer->buffer[i]);
	printf("\n");
	return length;
}

int write_frame(LinkLayer link_layer, char* buffer, unsigned int length) {
	unsigned int i;
	int ret = 0;
	char c = SERIAL_FLAG;

	if(write(link_layer->fd,&c,1) < 1)
		return -1;

	/* Byte stuffing */
		
	for(i = 0; i < length; ++i) {
		 switch(buffer[i]) {
            case SERIAL_FLAG:
            c = SERIAL_ESCAPE;
            if(write(link_layer->fd, &c,1) < 1)
            	return ret;
            c = SERIAL_FLAG_REPLACE;
            if(write(link_layer->fd, &c, 1) < 1)
            	return ret;
            break;
            case SERIAL_ESCAPE:
            c = SERIAL_ESCAPE;
            if(write(link_layer->fd, &c,1) < 1)
            	return ret;
            c = SERIAL_ESCAPE_REPLACE;
            if(write(link_layer->fd, &c,1) < 1)
            	return ret;
            break;
            default:
            if(write(link_layer->fd, &buffer[i], 1) < 1)
            	return ret;
            break;
        }
        printf("Wrote: %x\n", buffer[i]);
        ++ret;
	}
	
	c = SERIAL_FLAG;
	if(write(link_layer->fd,&c,1) < 1)
		return -1;

	return ret;
}

LinkLayer llinit(int port, int flag, unsigned int baudrate, unsigned int max_tries, unsigned int timeout, unsigned int max_frame_size) {	
	char port_name[20];
	sprintf(port_name, "/dev/ttyS%d", port);
	struct termios oldtio;
	
	int fd = open(port_name, O_RDWR | O_NOCTTY );
	if (fd < 0) {
		perror(port_name);
		return NULL;
	}

 	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    	perror("tcgetattr");
    	return NULL;
	}

	struct termios newtio;
	memset(&newtio, 0, sizeof(newtio));

	/* 	Control, input, output flags */
	newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD; /*  */
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = OPOST;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */


	if(tcflush(fd, TCIFLUSH) != 0)
		return NULL;

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
  		perror("tcsetattr");
  		return NULL;
	}
		printf("Port configured\n");

	/* Open the serial port for sending the message */
   
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = sig_alarm_handler;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
    	perror("Error: cannot handle SIGALRM");
    	return NULL;
	}

	LinkLayer link_layer = malloc(sizeof(struct LinkLayer_t));

	if(link_layer == NULL)
		return NULL;
	
	link_layer->fd = fd;
	link_layer->flag = flag;
	link_layer->baudrate = baudrate;
	link_layer->timeout = timeout;
	link_layer->max_tries = max_tries;
	link_layer->max_frame_size = max_frame_size;
	link_layer->sequence_number = 0;
	link_layer->buffer = malloc((max_frame_size-2)*sizeof(char));
	link_layer->oldtio = oldtio;

	if(link_layer->buffer == NULL){
		free(link_layer);
		return NULL;
	}
	
	return link_layer;
}

int llopen(LinkLayer link_layer){
    //signal(SIGALRM, sig_alarm_handler);
	if(link_layer->flag == TRANSMITTER)
  		return llopen_transmitter(link_layer);
	else if (link_layer->flag == RECEIVER)
  		return llopen_receiver(link_layer);
	else
  		return -1;
}

int llopen_transmitter(LinkLayer link_layer) {
	char set[] = {SERIAL_A_COM_TRANSMITTER,
       	SERIAL_C_SET,
       	SERIAL_A_COM_TRANSMITTER^SERIAL_C_SET};


  	printf("Transmitter open sequence\n");
    int length;
    reset_alarm();
    while (tries < link_layer->max_tries) {
    	printf("Sending SET\n");
      	write_frame(link_layer,set,3);
       	alarm(link_layer->timeout);
       	while (1) {
           	length = read_frame(link_layer);
           	if (length == -1)
               	break;
           	printf("Validating string\n");
           	if(is_valid_string(link_layer->buffer,length) && ua_validator(link_layer->buffer, length)) {
          		printf("Valid string\n"); 
              	alarm(0);
               	break;
           	}
       	}
       	if (length != -1)
           	break;
    }
    if (tries == link_layer->max_tries)    
       	return -1;

    return 0;
}

int llopen_receiver(LinkLayer link_layer) {
	printf("Receiver open sequence\n");

    char ua[] = {SERIAL_A_ANS_RECEIVER,
       SERIAL_C_UA,
       SERIAL_A_ANS_RECEIVER^SERIAL_C_UA};

    int length;
    while (1) {
        length = read_frame(link_layer);
        if (length <= 0)
            continue;        
        printf("Validating string\n");
        if(is_valid_string(link_layer->buffer,length) && set_validator(link_layer->buffer, length))
           break;
   }

   write_frame(link_layer, ua, 3);
   return 0;
}

int llclose(LinkLayer link_layer){
	
	if(link_layer->flag == TRANSMITTER)
		llclose_transmitter(link_layer);
	else if (link_layer->flag == RECEIVER)
		llclose_receiver(link_layer);
	else
		return -1;
	
	printf("Restoring port configurations\n");
	if (tcsetattr(link_layer->fd,TCSANOW,&(link_layer->oldtio)) == -1) {
       perror("tcsetattr");
       return -1;
   }
   close(link_layer->fd);

   return 0;
}

int llclose_transmitter(LinkLayer link_layer){
    char disc[] = {SERIAL_A_COM_TRANSMITTER,
        SERIAL_C_DISC,
        SERIAL_A_COM_TRANSMITTER^SERIAL_C_DISC};

    char ua[] = {
        SERIAL_A_ANS_RECEIVER,
        SERIAL_C_UA,
        SERIAL_A_ANS_RECEIVER^SERIAL_C_UA,
    };

    int length;
    reset_alarm();
    while (tries < link_layer->max_tries) {
    printf("Sending DISC\n");
    write_frame(link_layer,disc,3);
    alarm(3);
    while (1) {
        length = read_frame(link_layer);
        if (length <= 0)
            break;

        printf("Validating string\n");
        if(is_valid_string(link_layer->buffer,length) && disc_validator(link_layer->buffer, length)) {
            printf("Valid string\n"); 
            alarm(0);
            break;
        }
    }
    if (length != -1)
        break;
    }

    if (tries == link_layer->max_tries)    
        return -1;

    write_frame(link_layer, ua, 3);
    return 0;
}



int llclose_receiver(LinkLayer link_layer) {
     char disc[] = {SERIAL_A_ANS_RECEIVER,
       SERIAL_C_DISC,
       SERIAL_A_ANS_RECEIVER^SERIAL_C_DISC};

	/* Recepcao do DISC */

    int length;
    while (1) {
        length = read_frame(link_layer);
        if (length <= 0)
            continue;        
        printf("Validating string\n");
        if(is_valid_string(link_layer->buffer,length) && disc_validator(link_layer->buffer, length))
           break;
    }

    int length;
    reset_alarm();
    while (tries < link_layer->max_tries) {
    printf("Sending DISC\n");
    write_frame(link_layer,disc,3);
    alarm(3);
    while (1) {
        length = read_frame(link_layer);
        if (length <= 0)
            break;

        printf("Validating string\n");
        if(is_valid_string(link_layer->buffer,length) && ua_validator(link_layer->buffer, length)) {
            printf("Valid string\n"); 
            alarm(0);
            break;
        }
	}
   	if (length != -1)
    	break;
    }

    if (tries == link_layer->max_tries)    
        return -1;


    return 0;
}

unsigned int get_max_message_size(LinkLayer link_layer) {
	return link_layer->max_frame_size - 6;
}