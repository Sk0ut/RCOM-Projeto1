/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include "linklayer.h"

int main(int argc, char** argv)
{    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS5", argv[1])!=0) &&
	      (strcmp("/dev/ttyS4", argv[1])!=0))) {	
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS4\n");
      exit(1);
    }

    int port;
    sscanf(argv[1],"/dev/ttyS%d",&port); 

    int fd = llopen(port,TRANSMITTER);
    if (fd == -1) {
        printf("Unable to connect\n");
        return -1;    
    }
	
    llclose(fd, TRANSMITTER);


    return 0;
}
