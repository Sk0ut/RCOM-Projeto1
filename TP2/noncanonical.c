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
          (strcmp("/dev/ttyS4", argv[1])!=0) )) {   
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS4\n");
      exit(1);
    }
    
    int fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    printf("Opening port\n");
    llopen(fd,RECEIVER);
    printf("Port open\n");
   
    printf("Closing port\n");
    llclose(fd);
    printf("Port closed\n");

    return 0;
}
