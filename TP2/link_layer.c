#include "linklayer.h"

struct LinkLayer_t
{
	int fd;
	unsigned int baudrate;
	unsigned int time_out;
	unsigned int max_tries;
	unsigned int max_frame_size;
	int flag; // TRANSMITTER or RECEIVER
	struct termios oldtio;
	
	char * frame_buffer;
	unsigned int sequence_number;
};

LinkLayer llinit(int port, int flag, unsigned int baudrate, unsigned int max_tries, unsigned int time_out, unsigned int max_frame_size)
{	
	char port_name[20];
	sprintf(port_name, "/dev/ttyS%d", port);
	
	fd = open(port_name, O_RDWR | O_NOCTTY );
	if (fd < 0)
	{
		perror(port_name);
		return NULL;
	}


	LinkLayer link_layer = malloc(sizeof(LinkLayer_t));
	
	link_layer->flag = flag;
	link_layer->baudrate = baudrate;
	link_layer->time_out = time_out;
	link_layer->max_tries = max_tries;
	link_layer->max_frame_size = max_frame_size;
	
	link_layer->frame_buffer = malloc(sizeof(char) * max_frame_size);
	link_layer->sequence_number = 0;
	
	return link_layer;
}

unsigned int get_max_message_size(LinkLayer link_layer)
{
	return link_layer->max_frame_size - 6;
}