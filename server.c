#include "server.h"







struct client_info *last_client;
char password[MESSAGE_BUFFERSIZE];

char dest_chroot[PATH_MAX];
int is_dest_chroot = FALSE;

el *head = NULL; /* important- initialize to NULL! */


FILE *logfile;

	
/* ########################################################################################################## */
/* ***		Wrappers																					  *** */
/* ########################################################################################################## */

inline void receive_message_wrapper(int *socket, char *message_flag, char *message, struct client_info *client)
{
	int error;
	
	if ((error = receive_message(socket, message_flag, message)) != EXIT_SUCCESS) {
		print_log_message(client->ip_address, "Disconnected", NULL, FALSE);
		dealloc(client);
		pthread_exit(NULL);
	}
}

inline void recv_bytes_wrapper(char *file_attrs_buffer, int *socket, unsigned long long bytes, struct client_info *client)
{
	int error;
	
	if ((error = recv_bytes(file_attrs_buffer, socket, bytes)) != EXIT_SUCCESS) {
		print_log_message(client->ip_address, "Disconnected", NULL, FALSE);
		dealloc(client);
		pthread_exit(NULL);
	}
}

inline void send_message_wrapper(int *socket, char message_flag, char *message, struct client_info *client)
{
	int error;
	
	if ((error = send_message(socket, message_flag, message)) != EXIT_SUCCESS) {
		print_log_message(client->ip_address, "Disconnected", NULL, FALSE);
		dealloc(client);
		pthread_exit(NULL);
	}
}

inline void load_path_wrapper(long int *path_len, struct client_info *client, char *path, int *is_dest_chroot, char *dest_chroot)
{
	int error;
	long int dest_chroot_len = 0;
	
	if (*is_dest_chroot == TRUE) {
		dest_chroot_len = _strnlen(dest_chroot, PATH_MAX);
		memcpy(path, dest_chroot, dest_chroot_len);
		path[dest_chroot_len] = '/';
	}
	
	if ((error = load_path(path_len, &client->data_socket, path + dest_chroot_len + *is_dest_chroot)) != EXIT_SUCCESS) {
		print_log_message(client->ip_address, "Disconnected", NULL, FALSE);
		dealloc(client);
		pthread_exit(NULL);
	}
	
	if (strstr (path, "../") != NULL) {
		send_message_wrapper(&client->command_socket, FORCED_SOCKET_CLOSED, "BAD PATH", client);
		print_log_message(client->ip_address, "BAD PATH", path, TRUE);
		dealloc(client);
		pthread_exit(NULL);
	}
	
	*path_len += dest_chroot_len;
}

/* ########################################################################################################## */
/* ***		print_help: print the help when the server is executed with a wrong argument count			  *** */
/* ########################################################################################################## */

void print_help(char *program_name)
{
	printf("Usage:\n");
	printf("%s port logfile password [chroot_dir]\n\n", program_name);
	
	printf("'port' is port number (strongly recommended higher than 1024). The server will listen on this port\n");
	printf("'logfile' is a file used for log output. When 'logfile' is '-', logfile will be the stderr\n");
	printf("'password' is used for server-client authentication\n");
	printf("'chroot_dir' is optional. All incomming files can be chrooted in this chroot directory.\n");
}

/* ########################################################################################################## */
/* ***		print_log_message: print log message														  *** */
/* ########################################################################################################## */

inline void print_log_message(char *ip_address, char *message, char *path, char error)
{
	char timebuffer [80];
	time_t current_time;
	
	time(&current_time);
	strftime(timebuffer, 80, "%c", localtime(&current_time));
	
	if (path == NULL) {
		if (error == FALSE) {
			fprintf(logfile, "%s: Client %s %s\n", timebuffer, ip_address, message);
		} else {
			fprintf(logfile, "%s: Client %s Error: %s\n", timebuffer, ip_address, message);
		}
	} else {
		if (error == FALSE) {
			fprintf(logfile, "%s: Client %s: '%s' %s\n", timebuffer, ip_address, path, message);
		} else {
			fprintf(logfile, "%s: Client %s Error: '%s' %s\n", timebuffer, ip_address, path, message);
		}
	}
	
	fflush(logfile);
}

/* ########################################################################################################## */
/* ***		signal_handler: exits program when SIGINT or SIGTERM catched								  *** */
/* ########################################################################################################## */

void signal_handler(int sigNum)
{
	struct client_info *tmp;
	
	tmp = NULL;
	
	while (last_client) {
		close(last_client->command_socket);
		close(last_client->data_socket);
		close(last_client->server_socket);
		
		tmp = last_client;
		
		if (last_client->before == NULL) {
			free(tmp);
			break;
		} else {
			last_client = last_client->before;
		}
		free(tmp);
	}
	
	
	
	printf("\nInterupted by user. Exiting.\n");
	fprintf(logfile, "\nInterupted by user. Exiting.\n");
	
	fclose(logfile);
	exit(EXIT_SUCCESS);
}

/* ########################################################################################################## */
/* ***		dealloc: close open sockets and free client structures										  *** */
/* ########################################################################################################## */

inline void dealloc(struct client_info *client)
{
	close(client->command_socket);
	close(client->data_socket);
	
	if (client->next && client->before) {
		client->before->next = client->next;
		client->next->before = client->next->before;
	} else if (client->before) { /* last thread */
		last_client = client->before;
		client->before->next = NULL;
	} else if (client->next) { /* first thread */
		client->next->before = NULL;
	} else { /* all threads exited */
		last_client = NULL;
	}
	
	free(client);
}

/* ########################################################################################################## */
/* ***		load_dest_file_attrs: load message from client_daemon										  *** */
/* ########################################################################################################## */

inline int load_dest_file_attrs(char *dest_file_attrs_buffer, struct dest_file_attrs *dest_file_attrs, struct client_info *client)
{
	recv_bytes_wrapper(dest_file_attrs_buffer, &client->command_socket, MIN_DEST_FILE_ATTRS_LEN, client);
	
	dest_file_attrs->structure_len = dest_file_attrs_buffer[0];
	
	if (dest_file_attrs->structure_len == 0 || (dest_file_attrs->structure_len - MIN_DEST_FILE_ATTRS_LEN) < 0) {
		print_log_message(client->ip_address, "UNKNOWN PROTOCOL OR NETWORK FAILURE", NULL, TRUE);
		dealloc(client);
		pthread_exit(NULL);
	}
	
	recv_bytes_wrapper(dest_file_attrs_buffer + MIN_DEST_FILE_ATTRS_LEN, &client->command_socket, dest_file_attrs->structure_len - MIN_DEST_FILE_ATTRS_LEN, client);
	
	dest_file_attrs->command = dest_file_attrs_buffer[2];
	
	return EXIT_SUCCESS;
}

/* ########################################################################################################## */
/* ***		parse_dest_file_attrs: parsers file attributes												  *** */
/* ########################################################################################################## */

inline int parse_dest_file_attrs(char *dest_file_attrs_buffer, struct dest_file_attrs *dest_file_attrs, struct client_info *client)
{
	char *path_len_string, *data_len_string, *discard_len_string;
	int path_len_raw, data_len_raw;
	
	
	path_len_string = dest_file_attrs_buffer + 4;
	
	dest_file_attrs->path_len = strtol(path_len_string, NULL, 10);
	path_len_raw = _strnlen(path_len_string, MAX_HEADER_LEN);
	
	data_len_string = path_len_string + path_len_raw + 1;
	
	dest_file_attrs->data_len = strtoull(data_len_string, NULL, 10);
	data_len_raw = _strnlen(data_len_string, MAX_HEADER_LEN);
	
	discard_len_string = data_len_string + data_len_raw + 1;
	dest_file_attrs->discard = strtoull(discard_len_string, NULL, 10);
	
	/* CRC */
	if ((1 + 1 + 1 + 1 + path_len_raw + 1 + data_len_raw + 1 + _strnlen(discard_len_string, MAX_HEADER_LEN) + 1) != dest_file_attrs->structure_len) {
		print_log_message(client->ip_address, "UNKNOWN PROTOCOL OR NETWORK FAILURE", NULL, TRUE);
		dealloc(client);
		pthread_exit(NULL);
	}
	
	return EXIT_SUCCESS;
}

/* ########################################################################################################## */
/* ***		write_data: write data on hard drive														  *** */
/* ########################################################################################################## */

inline int write_data(char *data_buffer, struct dest_file_attrs *dest_file_attrs, FILE *file, struct client_info *client)
{
	unsigned long long remaining_bytes, i;
	
	remaining_bytes = dest_file_attrs->data_len % BUFFERSIZE;
	
	for (i=0; i < (dest_file_attrs->data_len / BUFFERSIZE); i++ ) {
		recv_bytes_wrapper(data_buffer, &client->data_socket, BUFFERSIZE, client);
		fwrite(data_buffer, BYTE_SIZE, BUFFERSIZE, file);
		if (ferror(file)) {
			send_message_wrapper(&client->command_socket, FORCED_SOCKET_CLOSED, strerror(errno), client);
			
			print_log_message(client->ip_address, strerror(errno), NULL, TRUE);
			print_log_message(client->ip_address, "fwrite FATAL ERROR, Disconnected", NULL, TRUE);
			
			dealloc(client);
			pthread_exit(NULL);
		}
	}
	
	if (remaining_bytes) {
		recv_bytes_wrapper(data_buffer, &client->data_socket, remaining_bytes, client);
		fwrite(data_buffer, BYTE_SIZE, remaining_bytes, file);
		if (ferror(file)) {
			send_message_wrapper(&client->command_socket, FORCED_SOCKET_CLOSED, strerror(errno), client);
			
			print_log_message(client->ip_address, strerror(errno), NULL, TRUE);
			print_log_message(client->ip_address, "fwrite FATAL ERROR, Disconnected", NULL, TRUE);
			
			dealloc(client);
			pthread_exit(NULL);
		}
	}
	
	return EXIT_SUCCESS;
}

/* ###################################################################################################################################### */
/* ***		discard_data: in case of non-critical failure (wrong file path etc.) this function discards useless data in the socket	  *** */
/* ###################################################################################################################################### */

inline int discard_data(struct dest_file_attrs *dest_file_attrs, struct client_info *client, char *discard_buffer)
{
	unsigned long long remaining_bytes, i;
	
	remaining_bytes = dest_file_attrs->discard % BUFFERSIZE;
	
	for (i=0; i < (dest_file_attrs->discard / BUFFERSIZE); i++ ) {
		recv_bytes_wrapper(discard_buffer, &client->data_socket, BUFFERSIZE, client);
	}
	
	if (remaining_bytes) {
		recv_bytes_wrapper(discard_buffer, &client->data_socket, remaining_bytes, client);
	}
	return EXIT_SUCCESS;
}

/* ########################################################################################################## */
/* ***		server_start: server socket initialization													  *** */
/* ########################################################################################################## */

void server_start (const int *port, int *server_socket)
{
	struct sockaddr_in socket_name;
	
	if ((*server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("FATAL ERROR socket");
		exit(EXIT_FAILURE);
	}
	
	socket_name.sin_family = AF_INET;
	socket_name.sin_port = htons(*port);
	socket_name.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(*server_socket, (struct sockaddr *)&socket_name, sizeof(socket_name)) == -1) {
		perror("FATAL ERROR bind");
		close(*server_socket);
		exit(EXIT_FAILURE);
	}
	
	if (listen (*server_socket, QUEUESIZE) == -1) {
		perror("FATAL ERROR listen");
		close(*server_socket);
		exit(EXIT_FAILURE);
	}
}

/*########################################################################################################### */
/* ***		authenticate: basic plaintext (not encrypted) authentication								  *** */
/* ########################################################################################################## */

int authenticate(struct client_info *client)
{
	char message[MESSAGE_BUFFERSIZE];
	char message_flag;
	int pass_len;
	
	receive_message(&client->command_socket, &message_flag, message);
	
	pass_len = _strnlen(password, MESSAGE_BUFFERSIZE);
	
	if ((strncmp (message, password, pass_len) != 0) || (pass_len != strlen(message))) {
		send_message(&client->command_socket, BAD_AUTH, "");
		return BAD_AUTH;
	} else {
		send_message(&client->command_socket, ACK_OK, "");
		return EXIT_SUCCESS;
	}
	
}

/* ########################################################################################################## */
/* ***		thr_func: every thread for every client_daemon												  *** */
/* ########################################################################################################## */

void thr_func(struct client_info *client)
{
	struct dest_file_attrs dest_file_attrs;
	char dest_file_attrs_buffer[MAX_HEADER_LEN], data_buffer[BUFFERSIZE];
	FILE *file;
	
	
	client->ip_address = inet_ntoa(client->command_info.sin_addr);
	
	print_log_message(client->ip_address, "connected to the server", NULL, FALSE);
	
	if (authenticate(client) != EXIT_SUCCESS) {
		print_log_message(client->ip_address, "BAD AUTH", NULL, TRUE);
		dealloc(client);
		return;
	}
	
	while (TRUE) {
		load_dest_file_attrs(dest_file_attrs_buffer, &dest_file_attrs, client);
		parse_dest_file_attrs(dest_file_attrs_buffer, &dest_file_attrs, client);
		
		if (dest_file_attrs.discard > 0) {
			discard_data(&dest_file_attrs, client, data_buffer);
		}
		
		load_path_wrapper(&dest_file_attrs.path_len, client, client->file_path, &is_dest_chroot, dest_chroot);
		
		switch (dest_file_attrs.command) {
			case MK_FILE:
				if (((file = fopen(client->file_path, "wb")) != NULL)) {
					write_data(data_buffer, &dest_file_attrs, file, client);
					fclose(file);
				} else {
					send_message_wrapper(&client->command_socket, ACK_ERROR, strerror(errno), client);
					print_log_message(client->ip_address, strerror(errno), client->file_path, TRUE);
					continue;
				}
				
				break;
			
			case MK_DIRECTORY:
				if (mkdir(client->file_path, 0777) == -1) {
					send_message_wrapper(&client->command_socket, ACK_ERROR, strerror(errno), client);
					print_log_message(client->ip_address, strerror(errno), client->file_path, TRUE);
					continue;
				}
				
				break;
			
			default:
				print_log_message(client->ip_address, "UNKNOWN PROTOCOL OR NETWORK FAILURE", NULL, TRUE);
				dealloc(client);
				return;
			}
			
		send_message_wrapper(&client->command_socket, ACK_OK, "", client);
		print_log_message(client->ip_address, "saved", client->file_path, FALSE);
	}
	
	dealloc(client);
}

/*########################################################################################################### */
/* ***		daemonize:																					  *** */
/* ########################################################################################################## */

void daemonize(void)
{
	int err;
	
	
	printf("Starting llncp server\n");
	fprintf(logfile, "Starting llncp server\n");
	fflush(logfile);
	
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

/*##################################################################################################################### */
/* ***		namecmp, accept_connection:		determine clients for deadlock prevetion (because 2 sockets on 1 port)  *** */
/* #################################################################################################################### */


int namecmp(el *a, el *b)
{
	return strcmp(a->token,b->token);
}


void accept_connection(int *server_socket, struct client_info *client)
{
	struct el *elt = NULL;
	int socket;
	char message_flag, message[MESSAGE_BUFFERSIZE];
	socklen_t addrlen = sizeof(struct sockaddr_in);
	
	
	while (TRUE) {
		struct el *connection = (struct el *) malloc(sizeof(struct el));
		
		if ((socket = accept(*server_socket, (struct sockaddr*)&connection->info, &addrlen)) == -1) {
			perror("FATAL ERROR accept");
			exit(EXIT_FAILURE);
		}
		
		receive_message(&socket, &message_flag, message);
		
		memcpy(connection->token, message, MESSAGE_BUFFERSIZE);
		connection->socket = socket;
		
		DL_SEARCH(head,elt,connection,namecmp);
		if (elt) {
			
			client->command_socket = elt->socket;
			client->data_socket = connection->socket;
			client->command_info = elt->info;
			client->data_info = connection->info;
			DL_DELETE(head,elt);
			return;
		} else {
			DL_APPEND(head, connection);
		}
	}
}



/* ########################################################################################################## */
/* ***		MAIN																						  *** */
/* ########################################################################################################## */

int main(int argc, char **argv)
{
	int server_socket;
	int port;
	pthread_attr_t attr;
	struct sigaction act;
	int thr = 0;
	
	
	memset(&act, 0, sizeof(act));
	act.sa_handler = signal_handler;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	
	if ((argc < 4) || (argc > 5)) {
		print_help(argv[0]);
		exit(EXIT_SUCCESS);
	}
	
	
	port = atoi(argv[1]);
	
	if (strcmp(argv[2], "-")) { 
		logfile = fopen(argv[2], "a");
		if (logfile == NULL) {
			perror("fopen logfile");
			exit(EXIT_FAILURE);
		}
	} else {
		logfile = stderr;
	}
	
	daemonize();
	
	memcpy(password, argv[3], MESSAGE_BUFFERSIZE);
	
		
	if (argc == 5) {
		is_dest_chroot = TRUE;
		memcpy(dest_chroot, argv[4], MESSAGE_BUFFERSIZE);
	}
	
	last_client = NULL;
	
	server_start(&port, &server_socket);
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);					   
	
	while (TRUE) {
		struct client_info *client = (struct client_info *) malloc(sizeof(struct client_info));
		
		accept_connection(&server_socket, client);
		
		client->thr_id = 0;
		client->server_socket = server_socket;
		client->thr=thr;
		
		/* linked list */
		client->next = NULL;
		client->before = last_client;
		if (client->before != NULL) {
			last_client->next = client;
		}
		last_client = client;
		
		/* thr_func(client); */
		pthread_create(&client->thr_id, &attr, (void*(*)(void*))thr_func, (void*)client);
		thr++;
	}
	pthread_attr_destroy(&attr); 
	
	close(server_socket);
	
	return 0;
}

