#include "utils.h"

int is_valid_s_u(const char* string);
int is_a_flag(const char a);
int is_c_flag(const char c);
int is_valid_bcc(const char a, const char c, const char bcc);
int is_valid_combination(const char a, const char c);
int is_valid_i(const char* string, int string_length);

int serial_read_string(int fd, char* string){
	int length = 0;
	char c;

	do{
		read(fd, &c, 1);
	} while(c != SERIAL_FLAG);

	while (length < 3)
	{	
		length = 0;
		read(fd, &c, 1);
		
		while(length < MAX_STRING_SIZE && c != SERIAL_FLAG){
			string[length] = c;
			++length;
			read(fd, &c, 1);
		}
	} 

	return length;
}

/* Validates string */
int is_valid_string(const char* string, const int string_length){
	if(string_length == 3){
		return is_valid_s_u(string);
	}
	else {
		return is_valid_i(string, string_length);
	}
}

/* Validatees su flag*/
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
		case SERIAL_A_COM_TRANSMITTER:
		case SERIAL_A_COM_RECEIVER:
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
			return (c|C_FLAG_R_VALUE)==SERIAL_C_RR || 
					(c|C_FLAG_R_VALUE)==SERIAL_C_REJ;
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

/* Validates I String*/
int is_valid_i(const char* string, int string_length){
	return FALSE;
}