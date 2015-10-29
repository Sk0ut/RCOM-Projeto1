#ifndef __LINKLAYER
#define __LINKLAYER

#define TRANSMITTER 0
#define RECEIVER 1

typedef struct LinkLayer_t * LinkLayer;

LinkLayer llinit(int port, int flag, unsigned int baudrate, unsigned int max_tries, unsigned int timeout, unsigned int max_frame_size);
int llopen(LinkLayer link_layer);
int llwrite(LinkLayer link_layer, char *buf, int length);
int llread(LinkLayer link_layer, char *buf);
int llclose(LinkLayer link_layer);
void lldelete(LinkLayer link_layer);
int get_max_message_size(LinkLayer link_layer);
void lllog(LinkLayer link_layer);

#endif // __LINKLAYER
