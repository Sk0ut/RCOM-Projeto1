#include "linklayer.h"

static int tries;

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
		res = read(fd, &c, 1);
		if (res < 1)
			return -1;
	} while(c != SERIAL_FLAG);

	while (length < 3) {	
		length = 0;

		do {
	        res = read(fd, &c, 1);
			if (res < 1)
				return -1;
			if (c == SERIAL_FLAG)
				break;

			switch(c){
	            case SERIAL_ESCAPE:
	            	res = read(link_layer->fd, &c ,1);
	            	if (res < 1)
	            		return -1; 
	                if(c == SERIAL_FLAG_REPLACE){
	                    link_layer->buffer[length] = SERIAL_FLAG;
	                }
	                else if(c == SERIAL_ESCAPE_REPLACE){
	                    link_layer->buffer[length] == SERIAL_ESCAPE;
	                }
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
	}

	return length;
}

int write_frame(LinkLayer link_layer, char* buffer, unsigned int length) {
	unsigned int i;
	int ret = 0;

	if(write(link_layer->fd,SERIAL_FLAG,1) < 1)
		return -1;

	/* Byte stuffing */
		
	for(i = 0; i < length; ++i) {
		 switch(buffer[i]) {
            case SERIAL_FLAG:
            if(write(link_layer->fd, SERIAL_ESCAPE,1) < 1)
            	return ret;
            if(write(link_layer->fd, SERIAL_FLAG_REPLACE, 1) < 1)
            	return ret;
            break;
            case SERIAL_ESCAPE:
            if(write(link_layer->fd, SERIAL_ESCAPE,1) < 1)
            	return ret;
            if(write(link_layer->fd, SERIAL_ESCAPE_REPLACE,1) < 1)
            	return ret;
            break;
            default:
            if(write(link_layer->fd, buffer[i], 1) < 1)
            	return ret;
            break;
        }
        ++ret;
	}
	
	if(write(link_layer->fd,SERIAL_FLAG,1) < 1)
		return -1;

	return ret;
}

LinkLayer llinit(int port, int flag, unsigned int baudrate, unsigned int max_tries, unsigned int timeout, unsigned int max_frame_size) {	
	char port_name[20];
	sprintf(port_name, "/dev/ttyS%d", port);
	struct termios oldtio;
	
	fd = open(port_name, O_RDWR | O_NOCTTY );
	if (fd < 0) {
		perror(port_name);
		return NULL;
	}

 	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    	perror("tcgetattr");
    	return -1;
	}

	struct termios newtio;
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

	LinkLayer link_layer = malloc(sizeof(LinkLayer_t));
	
	link_layer->flag = flag;
	link_layer->baudrate = baudrate;
	link_layer->timeout = timeout;
	link_layer->max_tries = max_tries;
	link_layer->max_frame_size = max_frame_size;
	link_layer->sequence_number = 0;
	link_layer->buffer = malloc((max_frame_size-2)*sizeof(char));
	link_layer->oldtio = oldtio;
	
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
           	if(is_valid_string(buf,length) && ua_validator(buf, length)) {
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

int llopen_receiver(int fd) {
	char buf[MAX_STRING_SIZE];
	printf("Receiver open sequence\n");
	printf("Reading from port\n");

    char ua[] = {SERIAL_A_ANS_RECEIVER,
       SERIAL_C_UA,
       SERIAL_A_ANS_RECEIVER^SERIAL_C_UA};

    int length;
    while (1) {
        length = read_frame(link_layer);
        if (length <= 0)
            continue;        
        printf("Validating string\n");
        if(is_valid_string(buf,length) && set_validator(buf, length))
           break;
   }

   write_frame(link_layer, ua, 3);
   return fd;
}

int llclose(LinkLayer link_layer){
	/*
	if(link_layer->flag == TRANSMITTER)
		llclose_transmitter(link_layer);
	else if (link_layer->flag == RECEIVER)
		llclose_receiver(link_layer);
	else
		return -1;
	*/
	printf("Restoring port configurations\n");
	if (tcsetattr(link_layer->fd,TCSANOW,&(link_layer->oldtio)) == -1) {
       perror("tcsetattr");
       return -1;
   }
   close(link_layer->fd);

   return 0;
}


int llclose_receiver(LinkLayer link_layer){
     char buf[MAX_STRING_SIZE];
     char disc[] = {SERIAL_FLAG,
       SERIAL_A_ANS_RECEIVER,
       SERIAL_C_DISC,
       SERIAL_A_ANS_RECEIVER^SERIAL_C_DISC,
       SERIAL_FLAG};

	/* Recepcao do DISC */

    int length;
    while (1) {
        length = read_frame(link_layer,buf);
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

unsigned int get_max_message_size(LinkLayer link_layer) {
	return link_layer->max_frame_size - 6;
}