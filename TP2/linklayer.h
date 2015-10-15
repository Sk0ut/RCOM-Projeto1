#ifndef __LINKLAYER
#define __LINKLAYER

#define TRANSMITTER 0
#define RECEIVER 1

int llopen(int port, int flag);
int llclose(int fd);

#endif // __LINKLAYER