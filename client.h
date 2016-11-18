#ifndef _CLIENT_H_
#define _CLIENT_H_

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
	#include <signal.h>
	#include <fcntl.h>
	
	#include "constants.h"
	#include "libs/libllncp.h"


	struct file_attrs {
		unsigned char file_attrs_len;
		long int source_path_len;
		long int destination_path_len;
		char *source_path;
		char *destination_path;
	} file_attrs;
	
	
	void print_help(char *program_name);
	inline void create_file_attrs(struct file_attrs *file_attrs, char *file_attrs_buffer);
	inline void client_connect(int *server_socket, const char *socket_path);
	
	


#endif /*! __CLIENT_H__ */

