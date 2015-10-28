#ifndef __UTILS
#define __UTILS

#define SERIAL_FLAG 0x7E
#define SERIAL_ESCAPE 0x7D
#define SERIAL_FLAG_REPLACE 0x5E
#define SERIAL_ESCAPE_REPLACE 0x5D

#define SERIAL_A_COM_TRANSMITTER 0x03
#define SERIAL_A_ANS_RECEIVER 0x03
#define SERIAL_A_COM_RECEIVER 0x01
#define SERIAL_A_ANS_TRANSMITTER 0x01
#define SERIAL_C_SET 0x07
#define SERIAL_C_DISC 0x0B
#define SERIAL_C_UA 0x03
#define SERIAL_C_RR_N1 0x21
#define SERIAL_C_RR_N0 0x01
#define SERIAL_C_REJ_N0 0x05
#define SERIAL_C_REJ_N1 0x25
#define SERIAL_I_C_N0	0x00
#define SERIAL_I_C_N1	0x20

#define SERIAL_SU_STRING_SIZE	5
#define MAX_STRING_SIZE	255

#define A_FLAG_INDEX	0
#define C_FLAG_INDEX	1
#define BCC_FLAG_INDEX 	2

#define C_FLAG_R_VALUE	0x20

#define RR0		0
#define RR1 	1
#define I0 		2
#define I1 		3

#define TRUE 1
#define FALSE 0

int serial_read_string(int fd, char* string);
int is_valid_string(const char* string, const int string_length);

#endif /* __UTILS */
