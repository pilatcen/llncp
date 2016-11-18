#ifndef _CLIENT_DAEMON_H_
#define _CLIENT_DAEMON_H_

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
	
	#include <sys/socket.h>
	#include <fcntl.h>
	#include <signal.h>
	#include <unistd.h>
	
	#include <time.h>

	#include "libs/try_throw_catch.h"
	#include "libs/struct_dest_file_attrs.h"
	#include "constants.h"
	#include "libs/libllncp.h"	
	
	
	
	
	struct file_attrs {
		unsigned char structure_len;
		long int source_path_len;
		long int destination_path_len;
		char source[PATH_MAX];
		char destination[PATH_MAX];
	} file_attrs;

	struct sockets {
		int command_socket;
		int data_socket;
		int server_socket;
		int client_socket;
	} sockets;

	inline void send_wrapper(int *socket, char *buffer, unsigned long long len);
	inline void fread_wrapper(char *buffer, unsigned long long len, FILE *file);
	inline void recv_bytes_wrapper(char *file_attrs_buffer, int *socket, unsigned long long bytes);
	inline void send_message_wrapper(int *socket, char message_flag, char *message);
	inline void send_bytes_check_message_flag_wrapper(char *buffer, int *write_socket, int *read_socket, unsigned long long bytes, char *message_flag, char *message, unsigned long long *sent_bytes);
	inline void load_path_wrapper(long int *path_len, int *socket, char *path);
	inline void send_path_wrapper(char *file_path, int *write_socket, long int *len);
	inline void receive_message_wrapper(int *socket, char *message_flag, char *message);
	
	inline int max(int *num1, int *num2, int *num3);
	void dealloc(void);
	void print_help(char *program_name);
	void signal_handler (int sig);
	void server_start (int *server_socket, char *server_address);
	void generate_token(char *token);
	int client_connect(const int *port, struct hostent *host);
	inline int load_file_attrs(struct file_attrs *file_attrs, char *file_attrs_buffer, int *socket);
	inline int create_dest_file_attrs(struct file_attrs *file_attrs, struct dest_file_attrs *dest_file_attrs, char *dest_file_attrs_buffer);
	inline int send_bytes_check_message(char *buffer, int *write_socket, int *server_read_socket, int *client_read_socket, unsigned long long bytes, char *message_flag, char *message, unsigned long long *sent_bytes);
	inline int send_data(struct dest_file_attrs *dest_file_attrs, struct sockets *sockets, char *buffer, int *write_socket, unsigned long long *len, FILE *file, char *message);
	void init(char *argv1, char *argv2, char *argv3, struct hostent *host, int *server_port, struct sockets *sockets, struct dest_file_attrs *dest_file_attrs);
	int authenticate(int *socket, char *password, char *message);
	void daemonize(void);
	void restart_connection(char *server_addr, struct sockets *sockets, int *server_port, char *password, char *message);


#endif /*! __CLIENT_DAEMON_H__ */
