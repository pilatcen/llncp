#ifndef SERVER_H
#define SERVER_H

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
	#include <time.h>
	
	#include <pthread.h>
	#include <fcntl.h>
	#include <signal.h>
	#include <inttypes.h>

	#ifdef __sun
		#include <thread.h>
	#endif
	
	#include "libs/utlist.h"

	#include "constants.h"
	#include "libs/struct_dest_file_attrs.h"
	#include "libs/libllncp.h"
	

	struct client_info {
		struct sockaddr_in command_info;
		struct sockaddr_in data_info;
		
		char *ip_address;
		
		int command_socket;
		int data_socket;
		
		int server_socket;
		
		pthread_t thr_id;
		
		char file_path[PATH_MAX];
		
		/* debug */
		int thr;
		
		struct client_info *next;
		struct client_info *before;
	} client_info;


	typedef struct el {
		char token[MESSAGE_BUFFERSIZE];
		int socket;
		struct sockaddr_in info;
		struct el *next, *prev;
	} el;



	inline void recv_bytes_wrapper(char *file_attrs_buffer, int *socket, unsigned long long bytes, struct client_info *client);
	inline void send_message_wrapper(int *socket, char message_flag, char *message, struct client_info *client);
	inline void load_path_wrapper(long int *path_len, struct client_info *client, char *path, int *is_dest_chroot, char *dest_chroot);

	void print_help(char *program_name);
	inline void print_log_message(char *ip_address, char *message, char *path, char error);
	void signal_handler(int sigNum);
	inline void dealloc(struct client_info *client);
	inline int load_dest_file_attrs(char *dest_file_attrs_buffer, struct dest_file_attrs *dest_file_attrs, struct client_info *client);
	inline int parse_dest_file_attrs(char *dest_file_attrs_buffer, struct dest_file_attrs *dest_file_attrs, struct client_info *client);
	inline int write_data(char *data_buffer, struct dest_file_attrs *dest_file_attrs, FILE *file, struct client_info *client);
	inline int discard_data(struct dest_file_attrs *dest_file_attrs, struct client_info *client, char *discard_buffer);
	void server_start (const int *port, int *server_socket);
	int authenticate(struct client_info *client);
	void thr_func (struct client_info *client);
	void daemonize(void);
	int namecmp(el *a, el *b);
	void accept_connection(int *server_socket, struct client_info *client);



#endif /*! __SERVER_H__ */
