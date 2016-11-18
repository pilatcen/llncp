#ifndef _LIBFFT_H_
#define _LIBFFT_H_

	#include <sys/un.h>
	#include <unistd.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <string.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <errno.h>
	#include <sys/select.h>
	#include <limits.h>
	
	#include "../constants.h"

	



	size_t _strnlen(const char *s, size_t max);
	void print_buffer(char *buffer, int byte_count, int type);
	inline int recv_bytes(char *buffer, int *socket, unsigned long long bytes);
	inline int receive_message(int *socket, char *message_flag, char *message);
	inline int send_bytes(char *buffer, int *write_socket, unsigned long long bytes);
	inline int send_path(char *path, int *write_socket, long int *len);
	inline int load_path(long int *path_len, int *socket, char *path);
	inline int send_message(int *socket, char message_flag, char *message);


#endif /*!_LIBFFT_H_*/
