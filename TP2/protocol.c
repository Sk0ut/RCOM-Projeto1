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
		    	SERIAL_A_COM_TRANSMITTER,
			    SERIAL_C_DISC,
			    SERIAL_A_COM_TRANSMITTER^SERIAL_C_DISC,
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

/* TODO: Byte stuffing, algoritmo de controlo de erros, retransmissao de tramas */

#define SERIAL_BCC2 0 /* Para efeitos de compilacao, isto e para remover dps */

int llwrite(int fd, char* buffer, int length){
    char buf[MAX_STRING_SIZE];

    int ret = 0;

    /* Preparacao do i0 */
    char* ibuf = malloc((length+6)*sizeof(char));

    ibuf[0] = SERIAL_FLAG;
    ibuf[1] = SERIAL_A_COM_TRANSMITTER;
    ibuf[2] = SERIAL_I_C_N0;
    ibuf[3] = SERIAL_A_COM_TRANSMITTER ^ SERIAL_I_C_N0;

    int i;
    for(i=4;i<length;++i){
        ibuf[i] == buffer[i-4];
    }

    ibuf[i] = SERIAL_BCC2; /* Perguntar a prof como raio e que isto se calcula */
    ibuf[i+1] = SERIAL_FLAG;

    /* Envio do i0 e resposta do recetor, sob a forma de um RR1 */
    int i_length;
    tries = 0;
    while (tries < 3) {
        printf("Sending i0 string \n");
        ret += write(fd, ibuf, i*sizeof(char));
        alarm(3);
        while (1) {
            i_length = serial_read_string(fd,buf);
            if (length == -1)
                break;

            printf("Validating string\n");
            if(is_valid_string(buf,i_length) && rr_validator(buf, i_length, RR1)) {
                printf("Valid string\n"); 
                alarm(0);
                break;
            }
        }
        if (i_length != -1)
            break;
    }

    if (tries == 3)    
        return -1;

    /* Preparacao do i1 */

    ibuf[0] = SERIAL_FLAG;
    ibuf[1] = SERIAL_A_COM_TRANSMITTER;
    ibuf[2] = SERIAL_I_C_N1;
    ibuf[3] = SERIAL_A_COM_TRANSMITTER ^ SERIAL_I_C_N1;

    for(i=4;i<length;++i){
        ibuf[i] == buffer[i-4];
    }

    ibuf[i] = SERIAL_BCC2; /* Perguntar a prof como raio e que isto se calcula */
    ibuf[i+1] = SERIAL_FLAG;

    /* Envio do i1 e resposta do recetor, sob a forma de um RR0 */
    tries = 0;
    while (tries < 3) {
        printf("Sending i1 string \n");
        ret += write(fd, ibuf, i*sizeof(char));
        alarm(3);
        while (1) {
            i_length = serial_read_string(fd,buf);
            if (length == -1)
                break;

            printf("Validating string\n");
            if(is_valid_string(buf,i_length) && rr_validator(buf, i_length, RR0)) {
                printf("Valid string\n"); 
                alarm(0);
                break;
            }
        }
        if (i_length != -1)
            break;
    }

    if (tries == 3)    
        return -1;

    return ret;
}

int llread(int fd, char* buffer){

    /* Eh pa, eu vou supor que o array ja vem alocado com o espaco certo, porque eu nao tenho forma
    absolutamente nenhuma de saber o tamanho do que vem ai antes de o receber. */

    char buf[MAX_STRING_SIZE];
    int length;
    int ret = 0;

    /* Receber o i0 */
    while (1) {
        length = serial_read_string(fd,buf);
        if (length <= 0)
            continue;        
        printf("Validating string\n");
        if(is_valid_string(buf,length) && is_valid_i(buf, length))
           break;
    }

    /* Parse da informacao do i0 */

    int i;

    for(i=4;i<length-2;++i){
        buffer[i-4] = buf[i];
    }

    ret+=(length-6);

    /* Enviar o RR1 */

    char rrbuf1[] = {SERIAL_FLAG,
                    SERIAL_A_ANS_RECEIVER,
                    SERIAL_C_RR_N1,
                    SERIAL_A_ANS_RECEIVER^SERIAL_C_RR_N1,
                    SERIAL_FLAG};


    write(fd,rrbuf1,5);

    /* Receber o i1 */

     while (1) {
        length = serial_read_string(fd,buf);
        if (length <= 0)
            continue;        
        printf("Validating string\n");
        if(is_valid_string(buf,length) && is_valid_i(buf, length))
           break;
    }

    /* Parse da informacao do i1 */

    int j;
    for(j=4;j<length-2;++i,++j){
        buffer[i] = buf[j];
    }

    ret+=(length-6);

    /* Enviar o RR0 */

    char rrbuf0[] = {SERIAL_FLAG,
                    SERIAL_A_ANS_RECEIVER,
                    SERIAL_C_RR_N0,
                    SERIAL_A_ANS_RECEIVER^SERIAL_C_RR_N0,
                    SERIAL_FLAG};


    write(fd,rrbuf0,5);

    return ret;
}