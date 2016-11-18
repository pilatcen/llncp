
#include "client_daemon.h"


/* All sockets in this struct */
struct sockets sockets;

/* For exceptions */
jmp_buf ex_buf__;

FILE *logfile;

char token[MESSAGE_BUFFERSIZE];

/* ########################################################################################################## */
/* ***		Wrappers																					  *** */
/* ########################################################################################################## */

inline void send_wrapper(int *socket, char *buffer, unsigned long long len)
{
	int error;
	
	error = send(*socket, buffer, len, 0);
	if (error == -1) {
		fprintf(logfile, "send: %s\n", strerror(errno));
		fflush(logfile);
		close(*socket);
		THROW(SOCKET_CLOSED);
	}
}
inline void fread_wrapper(char *buffer, unsigned long long len, FILE *file)
{
	fread(buffer, BYTE_SIZE, len, file);
	if (ferror(file)) {
		fclose(file);
		fprintf(logfile, "fread: %s\n", strerror(errno));
		fflush(logfile);
		THROW(FILE_ERROR);
	}
}

inline void recv_bytes_wrapper(char *file_attrs_buffer, int *socket, unsigned long long bytes)
{
	int error;
	
	if ((error = recv_bytes(file_attrs_buffer, socket, bytes)) != EXIT_SUCCESS) {
		THROW(error);
	}
}

inline void send_message_wrapper(int *socket, char message_flag, char *message)
{
	int error;
	
	if ((error = send_message(socket, message_flag, message)) != EXIT_SUCCESS) {
		/* kdyz klient zmackne ctrl+c, tak to chce poslat soubor znova*/
		THROW(FORCED_SOCKET_CLOSED);
	}
}

inline void load_path_wrapper(long int *path_len, int *socket, char *path)
{
	int error;
	
	if ((error = load_path(path_len, socket, path)) != EXIT_SUCCESS) {
		if (error == SOCKET_CLOSED) {
			THROW(FORCED_SOCKET_CLOSED);
		} else {
			THROW(error);
		}
	}
}

inline void send_path_wrapper(char *file_path, int *write_socket, long int *len)
{
	int error;
	
	if ((error = send_path(file_path, write_socket, len)) != EXIT_SUCCESS) {
		THROW(error);
	}
}

inline void receive_message_wrapper(int *socket, char *message_flag, char *message)
{
	int error;
	
	if ((error = receive_message(socket, message_flag, message)) != EXIT_SUCCESS) {
		THROW(error);
	}
}

/* ########################################################################################################## */
/* ***		max: determine maximum value from 3 vars													  *** */
/* ########################################################################################################## */

inline int max(int *num1, int *num2, int *num3)
{
	int tmp;
	
	tmp = *num1;
	if (tmp < *num2) {
		tmp = *num2;
	}
	if (tmp < *num3) {
		tmp = *num3;
	}
	return tmp;
}

/* ########################################################################################################## */
/* ***		dealloc: close all sockets and free memmory													  *** */
/* ########################################################################################################## */

void dealloc(void)
{
	close(sockets.command_socket);
	close(sockets.data_socket);
	close(sockets.server_socket);
	close(sockets.client_socket);
}

/* ########################################################################################################## */
/* ***		print_help: print the help when the client_daemon is executed with a wrong argument count	  *** */
/* ########################################################################################################## */

void print_help(char *program_name)
{
	printf("Usage:\n");
	printf("%s server_address server_port logfile [password]\n\n", program_name);
	
	printf("'server_address' is the address of the server where you can send your files\n");
	printf("'server_port' is an open port on your server with listening llncp-server\n");
	printf("'logfile' is a file used for log output\n");
	printf("'password' is used for server-client authentication. Password can be written as the 3rd argument; If not provided, the program will ask the password after its execution\n");
}


/* ########################################################################################################## */
/* ***		signal_handler: exits program when SIGINT or SIGTERM catched								  *** */
/* ########################################################################################################## */

void signal_handler(int sig)
{
	printf("\nInterupted by user. Exiting.\n");
	fprintf(logfile, "\nInterupted by user. Exiting.\n");
	dealloc();
	fclose(logfile);
	
	exit(EXIT_SUCCESS);
}

/* ########################################################################################################## */
/* ***		server_start: start the server part of client_daemon										  *** */
/* ########################################################################################################## */

void server_start(int *server_socket, char *server_address)
{
	int len;
	struct sockaddr_un local;
	char socket_path[PATH_MAX];
	
	if ((*server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		printf("socket");
		exit(EXIT_FAILURE);
	}
	
	strcpy(socket_path, SOCKET_ADDRESS);
	strcat(socket_path, server_address);
	
	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, socket_path);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);
	if (bind(*server_socket, (struct sockaddr *)&local, len) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(*server_socket, QUEUESIZE) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
}

/* ########################################################################################################## */
/* ***		generate_token: generate hostname and random based token									  *** */
/* ########################################################################################################## */

void generate_token(char *token)
{
	char hostname[MESSAGE_BUFFERSIZE];
	char number_char[MESSAGE_BUFFERSIZE];
	int  random_number;
	
	memset(token, 0, MESSAGE_BUFFERSIZE);
	
	random_number = rand();
	
	sprintf(number_char, "%d", random_number);
	gethostname(hostname, sizeof(hostname));
	
	strcat(token, hostname);
	strcat(token, number_char);
}

/* ########################################################################################################## */
/* ***		client_connect: connect to the server instance												  *** */
/* ########################################################################################################## */

int client_connect(const int *port, struct hostent *host)
{
	
	int client_socket;
	struct sockaddr_in server_socket;
	
	
	/* create socket */
	if ((client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		fprintf(logfile, "socket: %s\n", strerror(errno));
		fflush(logfile);
		return(-1);
	}
	
	server_socket.sin_family = AF_INET;
	server_socket.sin_port = htons(*port);
	memcpy(&(server_socket.sin_addr), host->h_addr, host->h_length);
	
	if (connect(client_socket, (struct sockaddr *)&server_socket, sizeof(server_socket)) == -1){
		fprintf(logfile, "connect: %s\n", strerror(errno));
		fflush(logfile);
		return(-1);
	}
	
	send_message_wrapper(&client_socket, AUTH_REQUEST, token);
	
	return client_socket;
}

/* ########################################################################################################## */
/* ***		load_file_attrs: receive and parse file_attrs from client in this format:					  *** */
/* ***			file_attrs_len\0source_path_len\0destination_path_len\0|source_path|destination_path	  *** */
/* ########################################################################################################## */

inline int load_file_attrs(struct file_attrs *file_attrs, char *file_attrs_buffer, int *socket)
{
	int source_path_len_raw, destination_path_len_raw;
	char *source_path_len_string, *destination_path_len_string;
	
	
	recv_bytes_wrapper(file_attrs_buffer, socket, MIN_FILE_ATTRS_LEN);
	
	file_attrs->structure_len = file_attrs_buffer[0];
	if (file_attrs->structure_len == 0 || (file_attrs->structure_len - MIN_FILE_ATTRS_LEN) < 0) {
		THROW(PROTOCOL_ERROR);
	}
	
	recv_bytes_wrapper(file_attrs_buffer + MIN_FILE_ATTRS_LEN, socket, file_attrs->structure_len - MIN_FILE_ATTRS_LEN);
	
	source_path_len_string = file_attrs_buffer + 2;
	source_path_len_raw = _strnlen(source_path_len_string, MAX_FILE_ATTRS_LEN);
	destination_path_len_string = source_path_len_string + source_path_len_raw + 1;
	destination_path_len_raw = _strnlen(destination_path_len_string, MAX_FILE_ATTRS_LEN);
	
	file_attrs->source_path_len = strtol(source_path_len_string, NULL, 10);
	file_attrs->destination_path_len = strtol(destination_path_len_string, NULL, 10);
	
	/* CRC */
	if (file_attrs->structure_len != (1 + 1 + source_path_len_raw + 1 + destination_path_len_raw + 1)) {
		THROW(PROTOCOL_ERROR);
	}
	
	return EXIT_SUCCESS;
}

/*########################################################################################################### */
/* ***		create_dest_file_attrs: creates message to server in this format:							  *** */
/* ***			command socket: dest_file_attrs_len\0command\0path_len\0data_len\0discard\0				  *** */
/* ***			data_socket: path|data																	  *** */
/* ########################################################################################################## */

inline int create_dest_file_attrs(struct file_attrs *file_attrs, struct dest_file_attrs *dest_file_attrs, char *dest_file_attrs_buffer)
{
	int sys_status;
	struct stat64 st_buf;
	int path_len_raw = 0;
	int data_len_raw = 0;
	int discard_len_raw = 0;
	char path_len[MAX_HEADER_LEN], data_len[MAX_HEADER_LEN], discard_len[MAX_HEADER_LEN];
	char zero = '\0';
	
	sys_status = stat64(file_attrs->source, &st_buf);
    if (sys_status != 0) {
        fprintf(logfile, "stat: %s\n", strerror(errno));
		send_message_wrapper(&sockets.client_socket, ACK_ERROR, strerror(errno));
		fflush(logfile);
    }
	
	dest_file_attrs->path_len = file_attrs->destination_path_len;
	
	if (S_ISDIR(st_buf.st_mode)) {
        dest_file_attrs->command = MK_DIRECTORY;
        dest_file_attrs->data_len = 0;
    }else{
		dest_file_attrs->command = MK_FILE;
		dest_file_attrs->data_len = (unsigned long long) st_buf.st_size;
	}
	
	data_len_raw = sprintf(data_len, "%llu", dest_file_attrs->data_len);
	path_len_raw = sprintf(path_len, "%ld", dest_file_attrs->path_len);
	discard_len_raw = sprintf(discard_len, "%llu", dest_file_attrs->discard);
	
	dest_file_attrs->structure_len = 1 + 1 + 1 + 1 + path_len_raw + 1 + data_len_raw + 1 + discard_len_raw + 1;
	
	memcpy(dest_file_attrs_buffer, &dest_file_attrs->structure_len, 1);
	memcpy(dest_file_attrs_buffer + 1, &zero, 1);
	memcpy(dest_file_attrs_buffer + 2, &dest_file_attrs->command, 1);
	memcpy(dest_file_attrs_buffer + 3, &zero, 1);
	strcpy (dest_file_attrs_buffer + 4, path_len);
	strcpy (dest_file_attrs_buffer + 4 + path_len_raw + 1, data_len);
	strcpy (dest_file_attrs_buffer + 4 + path_len_raw + 1 + data_len_raw + 1, discard_len);
	
	return EXIT_SUCCESS;
}

/*########################################################################################################### */
/* ***		send_bytes_check_message: sends data to server and checks when server or client sends message *** */
/* ########################################################################################################## */

inline int send_bytes_check_message(char *buffer, int *write_socket, int *server_read_socket, int *client_read_socket, unsigned long long bytes, char *message_flag, char *message, unsigned long long *sent_bytes)
{
	unsigned long long n;
	int retval;
	fd_set read_set, write_set;
	
	*sent_bytes = 0;
	
	while (*sent_bytes < bytes) {
		
		FD_ZERO(&write_set);
		FD_SET(*write_socket, &write_set);
		
		FD_ZERO(&read_set);
		FD_SET(*server_read_socket, &read_set);
		FD_SET(*client_read_socket, &read_set);
		
		retval = select (max(write_socket, server_read_socket, client_read_socket)+1, &read_set, &write_set, NULL, NULL);
		if (FD_ISSET(*write_socket, &write_set)) {
			n = send(*write_socket, buffer + *sent_bytes, bytes - *sent_bytes, 0);
			
			if ((n < 1) && (errno == EWOULDBLOCK)) {
				continue;
			} else if (n < 1) {
				fprintf(logfile, "send: %s\n", strerror(errno));
				THROW(SOCKET_CLOSED);
			}
			
			*sent_bytes += n;
			
		} else if (FD_ISSET(*server_read_socket, &read_set)) {
			receive_message_wrapper(server_read_socket, message_flag, message);
			return EXIT_SUCCESS;
		} else if (FD_ISSET(*client_read_socket, &read_set)) {
			if (receive_message(client_read_socket, message_flag, message) == SOCKET_CLOSED) {
				THROW(FORCED_SOCKET_CLOSED);
			}
			return EXIT_SUCCESS;
		} else if (retval == -1) {
			fprintf(logfile, "select: %s\n", strerror(errno));
			THROW(EXIT_FAILURE);
		}
	}
	
	return EXIT_SUCCESS;
}

/*########################################################################################################### */
/* ***		send_data: sends data to server																  *** */
/* ########################################################################################################## */

inline int send_data(struct dest_file_attrs *dest_file_attrs, struct sockets *sockets, char *buffer, int *write_socket, unsigned long long *len, FILE *file, char *message)
{
	unsigned long long remaining_bytes = *len % BUFFERSIZE;
	unsigned long long i, sent_bytes, totall_sent_bytes;
	char message_flag = ACK_OK;
	
	dest_file_attrs->discard = 0;
	sent_bytes = 0;
	totall_sent_bytes = 0;
	
	for (i = 0; i < (*len / BUFFERSIZE); i++) {
		fread_wrapper(buffer, BUFFERSIZE, file);
		send_bytes_check_message(buffer, &sockets->data_socket, &sockets->command_socket, &sockets->client_socket, BUFFERSIZE, &message_flag, message, &sent_bytes);
		totall_sent_bytes += sent_bytes;		
		if (message_flag != ACK_OK) {
			if (message_flag == ACK_ERROR) {
				dest_file_attrs->discard = totall_sent_bytes;
				fclose(file);
				THROW(RECEIVED_BAD_ACK);
			} else if (message_flag == FORCED_SOCKET_CLOSED) {
				send_message_wrapper(&sockets->client_socket, ACK_ERROR, message);
				fclose(file);
				THROW(FORCED_SOCKET_CLOSED);
			}
		}
	}
	
	if (remaining_bytes) {
		fread_wrapper(buffer, remaining_bytes, file);
		send_bytes_check_message(buffer, &sockets->data_socket, &sockets->command_socket, &sockets->client_socket, remaining_bytes, &message_flag, message, &sent_bytes);
		totall_sent_bytes += sent_bytes;
		if (message_flag != ACK_OK) {
			if (message_flag == ACK_ERROR) {
				dest_file_attrs->discard = totall_sent_bytes;
				fclose(file);
				THROW(RECEIVED_BAD_ACK);
			} else if (message_flag == FORCED_SOCKET_CLOSED) {
				send_message_wrapper(&sockets->client_socket, ACK_ERROR, message);
				fclose(file);
				THROW(FORCED_SOCKET_CLOSED);
			}
		}
	}
	
	return EXIT_SUCCESS;
}

/*########################################################################################################### */
/* ***		init: set up essential things when client_daemon starts										  *** */
/* ########################################################################################################## */

void init(char *argv1, char *argv2, char *argv3, struct hostent *host, int *server_port, struct sockets *sockets, struct dest_file_attrs *dest_file_attrs)
{
	int oldFlag;
	struct sigaction act1, act2;
	
	
	memset(&act1, 0, sizeof(act1));
	act1.sa_handler = signal_handler;
	
	sigaction(SIGINT, &act1, NULL);
	sigaction(SIGTERM, &act1, NULL);
	
	memset(&act2, 0, sizeof(act2));
	act2.sa_handler = SIG_IGN;
	
	/* Ingoring SIGPIPE */
	sigaction(SIGPIPE, &act2, NULL);
	
	if (strlen(argv1) > PATH_MAX) {
		printf("Server must have shorter address then %d chars.\n", PATH_MAX);
		exit(EXIT_FAILURE);
	}
	
	*server_port = atoi(argv2);
	
	if ((host = gethostbyname(argv1)) == NULL) {
		perror("gethostbyname");
		exit(EXIT_FAILURE);
	}
	
	logfile = fopen(argv3, "a");
	if (logfile == NULL) {
		perror("fopen logfile");
		exit(EXIT_FAILURE);
	}
	
	srand(time(NULL));
	generate_token(token);	
	
	sockets->command_socket = client_connect(server_port, host);
	sockets->data_socket = client_connect(server_port, host);
	
	if (sockets->command_socket == -1 || sockets->data_socket == -1) {
		exit(EXIT_FAILURE);
	}
	
	oldFlag = fcntl(sockets->data_socket, F_GETFL, 0);
	if (fcntl(sockets->data_socket, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
		perror("fcntl");
		exit(EXIT_FAILURE);
	}
	
	server_start(&sockets->server_socket, argv1);
	
	dest_file_attrs->discard = 0;
}

/*########################################################################################################### */
/* ***		authenticate: basic plaintext (not encrypted) authentication								  *** */
/* ########################################################################################################## */

int authenticate(int *socket, char *password, char *message)
{
	char message_flag;
	
	send_message(socket, AUTH_REQUEST, password);
	receive_message_wrapper(socket, &message_flag, message);
	
	if (message_flag == BAD_AUTH) {
		return BAD_AUTH;
	} else {
		fprintf(logfile, "Authenticated. Wainting for client connection.\n");
		fflush(logfile);
	}
	
	return EXIT_SUCCESS;
}

/*########################################################################################################### */
/* ***		daemonize:																					  *** */
/* ########################################################################################################## */

void daemonize(void)
{
	int err;
	
	printf("llncp-clientd started\n");
	
	/* daemonize */
	err = fork();
	if (err < 0) {
		/* fork error */
		exit(EXIT_FAILURE);
	}
	if (err > 0) {
		/* parent exits */
		exit(EXIT_SUCCESS);
	}
}

/*################################################################################################################### */
/* ***		restart_connection: If connection with server is broken (or clientd is forced to close connection)	  *** */
/* ***							this function will reconnect to server											  *** */
/* ################################################################################################################## */

void restart_connection(char *server_addr, struct sockets *sockets, int *server_port, char *password, char *message)
{
	
	struct hostent *host;
	
	
	fprintf(logfile, "server SOCKET CLOSED\n");
	fflush(logfile);
	if ((host = gethostbyname(server_addr)) == NULL) {
		fprintf(logfile, "gethostbyname: %s\n", strerror(errno));
		fflush(logfile);
		exit(EXIT_FAILURE);
	}
	
	close(sockets->command_socket);
	close(sockets->data_socket);
	
	sockets->command_socket = client_connect(server_port, host);
	sockets->data_socket = client_connect(server_port, host);
	
	while (sockets->command_socket == -1 || sockets->data_socket == -1) {
		fprintf(logfile, "Still trying to connect to the server %s\n", server_addr);
		fflush(logfile);
		
		sockets->command_socket = client_connect(server_port, host);
		sockets->data_socket = client_connect(server_port, host);
		
		sleep(3);
	}
	
	if (authenticate(&sockets->command_socket, password, message) != EXIT_SUCCESS) {
		fprintf(stderr, "Failed to authenticate");
		exit(EXIT_FAILURE);
	}
}


/*########################################################################################################### */
/* ***		main																						  *** */
/* ########################################################################################################## */

int main(int argc, char **argv)
{
	char dest_file_attrs_buffer[MAX_HEADER_LEN], file_attrs_buffer[MAX_FILE_ATTRS_LEN], data_buffer[BUFFERSIZE];
	
	int server_port;
	struct hostent *host;
	FILE *file;
	
	struct dest_file_attrs dest_file_attrs;
	struct file_attrs file_attrs;
	
	char password[MESSAGE_BUFFERSIZE];
	char message[MESSAGE_BUFFERSIZE];
	char message_flag;
	socklen_t addrlen;
	struct sockaddr_in server_info;
	
	char file_resend_needed;
	
	addrlen = sizeof(struct sockaddr_in);
	host = NULL;
	file = NULL;
	file_resend_needed = FALSE;
	
	
	if ((argc < 4) || (argc > 5)) {
		print_help(argv[0]);
		exit(EXIT_SUCCESS);
	}
	
	if (argc == 4) {
		printf("Enter password: ");
		fgets(password, MESSAGE_BUFFERSIZE - 1, stdin);
		if (ferror(stdin)) {
			perror("fgets");
			exit(EXIT_FAILURE);
		}
		password[_strnlen(password, MESSAGE_BUFFERSIZE) - 1] = '\0';
	} else {
		strcpy(password, argv[4]);
	}
	
	init(argv[1], argv[2], argv[3], host, &server_port, &sockets, &dest_file_attrs);
	
	if (authenticate(&sockets.command_socket, password, message) != EXIT_SUCCESS) {
		fprintf(stderr, "Wrong password.\n");
		exit(EXIT_FAILURE);
	}
	
	
	daemonize();
	
	
	while (TRUE) {
		message_flag = ACK_OK;
		
		if (file_resend_needed == FALSE) {
			if ((sockets.client_socket = accept(sockets.server_socket, (struct sockaddr *)&server_info,  (socklen_t *) &addrlen)) == -1) {
				perror("accept");
				exit(EXIT_FAILURE);
			}
		}
		
		TRY {
			if (file_resend_needed == FALSE) {
				load_file_attrs(&file_attrs, file_attrs_buffer, &sockets.client_socket);
				
				load_path_wrapper(&file_attrs.source_path_len, &sockets.client_socket, file_attrs.source);
				load_path_wrapper(&file_attrs.destination_path_len, &sockets.client_socket, file_attrs.destination);
				
				create_dest_file_attrs(&file_attrs, &dest_file_attrs, dest_file_attrs_buffer);
			} else {
				file_resend_needed = FALSE;
			}
			
			if (dest_file_attrs.command == MK_FILE) {
				file = fopen(file_attrs.source, "rb");
				if (file == NULL) {
					send_message_wrapper(&sockets.client_socket, ACK_ERROR, strerror(errno));
					fprintf(logfile, "fopen: %s\n", strerror(errno));
					THROW(FILE_ERROR);
				}
				send_wrapper(&sockets.command_socket, dest_file_attrs_buffer, dest_file_attrs.structure_len);
				send_path_wrapper(file_attrs.destination, &sockets.data_socket, &dest_file_attrs.path_len);
				send_data(&dest_file_attrs, &sockets, data_buffer, &sockets.data_socket, &dest_file_attrs.data_len, file, message);
				fclose(file);
			} else {
				send_wrapper(&sockets.command_socket, dest_file_attrs_buffer, dest_file_attrs.structure_len);
				send_path_wrapper(file_attrs.destination, &sockets.data_socket, &dest_file_attrs.path_len);
			}
			
			receive_message_wrapper(&sockets.command_socket, &message_flag, message);
			
			if (message_flag == ACK_ERROR) {
				dest_file_attrs.discard = dest_file_attrs.data_len;
				THROW(RECEIVED_BAD_ACK);
			} else if (message_flag == FORCED_SOCKET_CLOSED) {
				send_message_wrapper(&sockets.client_socket, ACK_ERROR, message);
				THROW(FORCED_SOCKET_CLOSED);
			}
			
			send_message_wrapper(&sockets.client_socket, message_flag, message);
			close(sockets.client_socket);
		}
		
		CATCH(FILE_ERROR) {
			fprintf(logfile, "FILE_ERROR\n");
			fflush(logfile);
			close(sockets.client_socket);
		}
		CATCH(RECEIVED_BAD_ACK) {
			fprintf(logfile, "Message from server: %s\n", message);
			send_message_wrapper(&sockets.client_socket, ACK_ERROR, message);
			fflush(logfile);
			close(sockets.client_socket);
		}
		CATCH(SOCKET_CLOSED) {
			dest_file_attrs.discard = 0;
			file_resend_needed = TRUE;
			restart_connection(argv[1], &sockets, &server_port, password, message);
			
		}
		CATCH(FORCED_SOCKET_CLOSED) {
			file_resend_needed = FALSE;
			close(sockets.client_socket);
			restart_connection(argv[1], &sockets, &server_port, password, message);
		}
		CATCH(PROTOCOL_ERROR) {
			fprintf(logfile, "PROTOCOL ERROR\n");
			fflush(logfile);
			close(sockets.client_socket);
		}
		CATCH(BAD_AUTH) {
			fprintf(logfile, "BAD_AUTH\n");
			fflush(logfile);
			exit(EXIT_FAILURE);
		}
		CATCH(EXIT_FAILURE) {
			fprintf(logfile, "EXIT_FAILURE\n");
			if (file != NULL) {
				fclose(file);
			}
			fclose(logfile);
			dealloc();
			exit(EXIT_FAILURE);
		}
		ETRY;
	}
	
	close(sockets.command_socket);
	close(sockets.data_socket);
	
	return 0;
}



