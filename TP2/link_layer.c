#include "linklayer.h"
#include "utils.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static int tries;
static int timeoutNo;

int is_valid_s_u(const char* string);
int is_a_flag(const char a);
int is_c_flag(const char c);
int is_valid_bcc(const char a, const char c, const char bcc);
int is_valid_combination(const char a, const char c);
int is_valid_i(const char* string, int string_length);

int llopen_receiver(LinkLayer link_layer);
int llopen_transmitter(LinkLayer link_layer);
int llclose_transmitter(LinkLayer link_layer);
int llclose_receiver(LinkLayer link_layer);



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
	int rejNo;
	int iNo;
};

int generate_error(){
	return (rand()%5 == 0);
}

/* Validates string */
int is_valid_string(const char* string, const int string_length){
	if(string_length == 3)
		return is_valid_s_u(string);
	else 
		return is_valid_i(string, string_length);
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
		case 0x01:
		case 0x03:
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
			return (c|C_FLAG_R_VALUE)==SERIAL_C_RR_N1 ||
				   (c|C_FLAG_R_VALUE)==SERIAL_C_REJ_N1 ||
				   (c|C_FLAG_R_VALUE)==SERIAL_I_C_N1;
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

/* Validates I String Header */
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

	return TRUE;
}

int i_valid_bcc2(LinkLayer link_layer, char* buffer, int string_length){
	int bcc2 = 0;
	int i;

	for(i = 3; i < string_length - 1; ++i)
		bcc2 ^= buffer[i];

	return bcc2 == buffer[string_length-1];
}

int is_expected_i(LinkLayer link_layer, char* buffer){
	if((buffer[C_FLAG_INDEX] == SERIAL_I_C_N0) && (link_layer->sequence_number == 0))
		return TRUE;

	if((buffer[C_FLAG_INDEX] == SERIAL_I_C_N1) && (link_layer->sequence_number == 1))
		return TRUE;

	return FALSE;
}

int ua_validator(int flag, char* buffer, int length){
    int aFlag;

    if(flag == TRANSMITTER)
    	aFlag = SERIAL_A_ANS_RECEIVER;
    else
    	aFlag = SERIAL_A_ANS_TRANSMITTER;

    return 	(length == 3) &&
    		(buffer[A_FLAG_INDEX] == aFlag) &&
    		(buffer[C_FLAG_INDEX] == SERIAL_C_UA);
}

int set_validator (int flag, char* buffer, int length){
	int aFlag;

	if(flag == TRANSMITTER)
    	aFlag = SERIAL_A_COM_RECEIVER;
    else
    	aFlag = SERIAL_A_COM_TRANSMITTER;

    return 	(length == 3) &&
    		(buffer[A_FLAG_INDEX] == aFlag) &&
    		(buffer[C_FLAG_INDEX] == SERIAL_C_SET);
}

int disc_validator (int flag, char* buffer, int length){
    int aFlag;

	if(flag == TRANSMITTER)
    	aFlag = SERIAL_A_COM_RECEIVER;
    else
    	aFlag = SERIAL_A_COM_TRANSMITTER;

    return 	(length == 3) &&
    		(buffer[A_FLAG_INDEX] == aFlag) &&
    		(buffer[C_FLAG_INDEX] == SERIAL_C_DISC);
}

int rr_validator (char* buffer, int length){
    return (length == 3) && ((buffer[C_FLAG_INDEX] == SERIAL_C_RR_N0) || (buffer[C_FLAG_INDEX] == SERIAL_C_RR_N1));
}

int rej_validator (char* buffer, int length){
	return  (length == 3) && ((buffer[C_FLAG_INDEX] == SERIAL_C_REJ_N0) || (buffer[C_FLAG_INDEX] == SERIAL_C_REJ_N1));
}

void sig_alarm_handler(int sig) {
    if (sig == SIGALRM) {
        ++tries;
		++timeoutNo;
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

		if (length == link_layer->max_frame_size) {
			do {
				res = read(link_layer->fd, &c, 1);
				if (res < 1)
					return -1;
			} while(c != SERIAL_FLAG);
		}
	}

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
	srand(time(NULL));
	
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

	/* Open the serial port for sending the message */
   
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = sig_alarm_handler;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
    	perror("Error: cannot handle SIGALRM");
    	return NULL;
	}

	if(max_frame_size <= 6){
		printf("Error: Maximum frame size too small\n");
		return NULL;
	}

	if(max_frame_size > 65536){
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
	link_layer->rejNo = 0;
	link_layer->iNo = 0;

	if(link_layer->buffer == NULL){
		free(link_layer);
		return NULL;
	}

	timeoutNo = 0;
	
	return link_layer;
}

int llopen(LinkLayer link_layer){
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

    int length;
    reset_alarm();
    while (tries < link_layer->max_tries) {
      	write_frame(link_layer,set,3);
       	alarm(link_layer->timeout);
       	while (1) {
           	length = read_frame(link_layer);
           	if (length == -1)
               	break;
           	if(is_valid_string(link_layer->buffer,length) && ua_validator(link_layer->flag, link_layer->buffer, length)) {
              	alarm(0);
               	break;
           	}
       	}
       	if (length != -1)
           	break;
    }
	alarm(0);
    if (tries == link_layer->max_tries)    
       	return -1;

    return 0;
}

int llopen_receiver(LinkLayer link_layer) {

    char ua[] = {SERIAL_A_ANS_RECEIVER,
       SERIAL_C_UA,
       SERIAL_A_ANS_RECEIVER^SERIAL_C_UA};

    int length;
	
	alarm(link_layer->max_tries * link_layer->timeout);
    while (tries == 0) {
        length = read_frame(link_layer);
        if (length <= 0)
            continue;        
        if(is_valid_string(link_layer->buffer,length) && set_validator(link_layer->flag, link_layer->buffer, length))
           break;
   }
	alarm(0);
	if (length <= 0)
	return -1; 

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
        SERIAL_A_ANS_TRANSMITTER,
        SERIAL_C_UA,
        SERIAL_A_ANS_TRANSMITTER^SERIAL_C_UA,
    };

    int length;
    reset_alarm();
    while (tries < link_layer->max_tries) {
   		write_frame(link_layer,disc,3);
  		alarm(link_layer->timeout);
   		while (1) {
        	length = read_frame(link_layer);
        	if (length <= 0)
           	 	break;

	        if(is_valid_string(link_layer->buffer,length) && disc_validator(link_layer->flag, link_layer->buffer, length)) {
    	        alarm(0);
    	        break;
        	}
    	}
    	if (length > 0)
        	break;
    }
	alarm(0);
    if (tries == link_layer->max_tries)    
        return -1;

    write_frame(link_layer, ua, 3);
    return 0;
}

void lllog(LinkLayer link_layer){
	printf("Number of timeouts occured: %d\n",timeoutNo);
	printf("Number of REJ frames %s: %d\n", link_layer->flag == TRANSMITTER ? "received": "transmitted", link_layer->rejNo);
	printf("Number of I frames %s: %d\n", link_layer->flag == RECEIVER ? "received": "transmitted", link_layer->iNo);
}

int llclose_receiver(LinkLayer link_layer) {
    char disc[] = {SERIAL_A_COM_RECEIVER, SERIAL_C_DISC, SERIAL_A_COM_RECEIVER^SERIAL_C_DISC};
    int length;

	reset_alarm();
	alarm(link_layer->max_tries * link_layer->timeout);
    while (tries == 0) {
        length = read_frame(link_layer);
        if (length <= 0)
            continue;        
        if(is_valid_string(link_layer->buffer,length) && disc_validator(link_layer->flag, link_layer->buffer, length))
           break;
    }
	alarm(0);
	if (length <= 0)
		return -1;

    reset_alarm();
    while (tries < link_layer->max_tries) {
		write_frame(link_layer,disc,3);
		alarm(link_layer->timeout);
		while (1) {
		    length = read_frame(link_layer);
		    if (length <= 0)
		        break;

		    if(is_valid_string(link_layer->buffer,length) && ua_validator(link_layer->flag, link_layer->buffer, length)) {
		        alarm(0);
		        break;
		    }
		}
	   	if (length > 0)
			break;
    }

    if (tries == link_layer->max_tries)    
        return -1;


    return 0;
}

int llread(LinkLayer link_layer, uint8_t *buf){
	int length;
	char ans[3];

	reset_alarm();
	alarm(link_layer->max_tries * link_layer->timeout);
    while (tries == 0) {
        length = read_frame(link_layer);
        if (length <= 0)
            continue;

        if(is_valid_string(link_layer->buffer,length) == FALSE)
        	continue;
		
		if(length == 3) {
			if(set_validator(link_layer->flag, link_layer->buffer, length)){
				ans[0] = SERIAL_A_ANS_RECEIVER;
				ans[1] = SERIAL_C_UA;
				ans[2] = ans[0] ^ ans[1];
				write_frame(link_layer, ans, 3);
			}
			continue;
		}

		++link_layer->iNo;		

	    if(i_valid_bcc2(link_layer, link_layer->buffer,length) && is_expected_i(link_layer, link_layer->buffer) && !generate_error())    
			break;

		ans[0] = SERIAL_A_ANS_RECEIVER;
    	if(is_expected_i(link_layer, link_layer->buffer)){
    		ans[1] = link_layer->sequence_number == 0 ? SERIAL_C_REJ_N0 : SERIAL_C_REJ_N1;
			++link_layer->rejNo;
		}
    	else
    		ans[1] = link_layer->sequence_number == 0 ? SERIAL_C_RR_N0 : SERIAL_C_RR_N1;
    	ans[2] = ans[0] ^ ans[1];

    	write_frame(link_layer, ans, 3);
		
   	}
	alarm(0);
	if (length <= 0)
		return -1;

	memcpy(buf, &(link_layer->buffer[3]), length-4);
	
   	ans[0] = SERIAL_A_ANS_RECEIVER;
   	ans[1] = link_layer->sequence_number == 0 ? SERIAL_C_RR_N1 : SERIAL_C_RR_N0;
	ans[2] = ans[0] ^ ans[1];

	write_frame(link_layer,ans,3);

	link_layer->sequence_number = 1 - link_layer->sequence_number;

   	return length-4;
}

int llwrite(LinkLayer link_layer, uint8_t* buf, int length){
	int iLength = length+4;
	char frame[iLength];

	frame[0] = SERIAL_A_COM_TRANSMITTER;
	frame[1] = link_layer->sequence_number == 0 ? SERIAL_I_C_N0 : SERIAL_I_C_N1;
	frame[2] = frame[0] ^ frame[1];

	int bufCounter;
	char bcc2 = 0;
	for(bufCounter = 0; bufCounter < length; bufCounter++){
		frame[3+bufCounter] = buf[bufCounter];
		bcc2^=buf[bufCounter];
	}
	frame[iLength-1] = bcc2;
	
	int ansLength;
	int resend;
	reset_alarm();
    while (tries < link_layer->max_tries) {
   		write_frame(link_layer,frame,iLength);
		++link_layer->iNo;
   		resend = FALSE;
    	alarm(3);
    	while (1) {
        	ansLength = read_frame(link_layer);
        	if (ansLength <= 0){
        		resend = TRUE;
        		break;
        	}

        	if(!is_valid_string(link_layer->buffer,ansLength))
        		continue;
        	if(ansLength != 3)
        		continue;
        	if(rr_validator(link_layer->buffer, ansLength)) {
				break;
			}
        	if(rej_validator(link_layer->buffer, ansLength)){
				++link_layer->rejNo;
        		if(link_layer->sequence_number == 0 && link_layer->buffer[C_FLAG_INDEX] == SERIAL_C_REJ_N0){
        			resend = TRUE;
        			break;
        		}
        		if(link_layer->sequence_number == 1 && link_layer->buffer[C_FLAG_INDEX] == SERIAL_C_REJ_N1){
        			resend = TRUE;
        			break;
        		}
        		continue;
        	}

		}
		if (resend)
			continue; 

		if(link_layer->sequence_number == 0 && link_layer->buffer[C_FLAG_INDEX] == SERIAL_C_RR_N1)
        	break;
        if(link_layer->sequence_number == 1 && link_layer->buffer[C_FLAG_INDEX] == SERIAL_C_RR_N0)
        	break;
   	}
   	alarm(0);

   	if (tries == link_layer->max_tries)    
       	return -1;

    link_layer->sequence_number = 1 - link_layer->sequence_number;
    return length;
}

void lldelete(LinkLayer link_layer) {
	if (link_layer) {
		if (link_layer->buffer)
			free(link_layer->buffer);
		free(link_layer);
	}
}

int get_max_message_size(LinkLayer link_layer) {
	return link_layer->max_frame_size - 6;
}
