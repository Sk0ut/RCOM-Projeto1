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
#include "utils.h"

#define BAUDRATE B38400

int main(int argc, char** argv) {    
    if ( (argc < 2) || 
         ((strcmp("/dev/ttyS0", argv[1])!=0) && 
          (strcmp("/dev/ttyS4", argv[1])!=0) &&
    (strcmp("/dev/ttyS5", argv[1])!=0) )) {   
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS4\n");
      exit(1);
    }
    
    int port;
    sscanf(argv[1],"/dev/ttyS%d",&port); 

    LinkLayer link_layer = llinit(port, TRANSMITTER, BAUDRATE, 3, 3, MAX_STRING_SIZE);
    if(linklayer == NULL)
      return 1;

    if (llopen(link_layer) == -1)
      return 1;
    printf("Port open\n");
   
    printf("Closing port\n");
    if (llclose(link_layer))
      return 1;
    printf("Port closed\n");

    return 0;
}
