#include "client.h"





int server_socket;

/* ############################################################################################################# */
/* ***		print_help: prints the help message when the client is executed with an incorrect argument count *** */
/* ############################################################################################################# */

void print_help(char *program_name)
{
	printf("Usage:\n");
	printf("%s server_address source destination\n\n", program_name);
	
	printf("'server_address' is server with running LLCP server\n");
	printf("'source' is absolute path to file you want to send\n");
	printf("'destination' is absolute path to the remote location\n");
}

/* ########################################################################################################## */
/* ***		signal_handler: exits program when SIGINT or SIGTERM catched								  *** */
/* ########################################################################################################## */

void signal_handler (int sig)
{
	int oldFlag;
	
	printf("\nInterupted by user. Exiting.\n");
	
	oldFlag = fcntl(server_socket, F_GETFL, 0);
	if (fcntl(server_socket, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
		perror("fcntl");
		exit(EXIT_FAILURE);
	}
	
	send_message(&server_socket, FORCED_SOCKET_CLOSED, "");
	
	exit(EXIT_SUCCESS);
}

/*########################################################################################################### */
/* ***		create_file_attrs: creates message to server in this format:								  *** */
/* ***			file_attrs_len\0source_path_len\0destination_path_len\0|source_path|destination_path	  *** */
/* ########################################################################################################## */

inline void create_file_attrs(struct file_attrs *file_attrs, char *file_attrs_buffer)
{
	char source_path_len_string[MAX_FILE_ATTRS_LEN], destination_path_len_string[MAX_FILE_ATTRS_LEN];
	int source_path_len_raw, destination_path_len_raw, tmp;
	
	source_path_len_raw = sprintf(source_path_len_string, "%ld", file_attrs->source_path_len);
	destination_path_len_raw = sprintf(destination_path_len_string, "%ld", file_attrs->destination_path_len);
	
	/* dest_file_attrs size */
	file_attrs->file_attrs_len = 1 + 1 + source_path_len_raw + 1 + destination_path_len_raw + 1;
	file_attrs_buffer[0] = file_attrs->file_attrs_len;
	file_attrs_buffer[1] = '\0';
	
	/* source path length */
	strcpy(file_attrs_buffer + 2, source_path_len_string);
	
	/* destination path length */
	tmp = 2 + source_path_len_raw + 1;
	strcpy (file_attrs_buffer + tmp, destination_path_len_string);
}

/* ########################################################################################################## */
/* ***		client_connect: connect to the client_socket instance										  *** */
/* ########################################################################################################## */

inline void client_connect(int *server_socket, const char *socket_path)
{
	socklen_t addrlen;
	struct sockaddr_un remote;
	
	if ((*server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	
	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, socket_path);
	addrlen = strlen(remote.sun_path) + sizeof(remote.sun_family);
	if (connect(*server_socket, (struct sockaddr *)&remote, addrlen) == -1) {
		close(*server_socket);
		perror("connect");
		exit(EXIT_FAILURE);
	}
}

/* ########################################################################################################## */
/* ***		main																						  *** */
/* ########################################################################################################## */

int main(int argc, char **argv)
{
	
	struct file_attrs file_attrs;
	char socket_path[PATH_MAX], file_attrs_buffer[MAX_FILE_ATTRS_LEN];
	char message[MESSAGE_BUFFERSIZE];
	char message_flag;
	struct sigaction act;
	
	
	memset(&act, 0, sizeof(act));
	act.sa_handler = signal_handler;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	
	
	if (argc != 4) {
		print_help(argv[0]);
		exit(EXIT_SUCCESS);
	}
	
	if ((_strnlen(argv[1], (PATH_MAX + 1))) > PATH_MAX) {
		fprintf(stderr, "Server must have shorter address than %d chars.\n", PATH_MAX);
		exit(EXIT_SUCCESS);
	}
	
	file_attrs.source_path = argv[2];
	file_attrs.destination_path = argv[3];
	file_attrs.source_path_len = _strnlen(file_attrs.source_path, PATH_MAX) + 1;
	file_attrs.destination_path_len = _strnlen(file_attrs.destination_path, PATH_MAX) + 1;
	
	strcpy (socket_path, SOCKET_ADDRESS);
	strcat (socket_path, argv[1]);
	
	client_connect(&server_socket, socket_path);
	create_file_attrs(&file_attrs, file_attrs_buffer);
	
	if (send(server_socket, file_attrs_buffer, file_attrs.file_attrs_len, 0) == -1) {
		close(server_socket);
		perror("send");
		exit(EXIT_FAILURE);
	}
	
	send(server_socket, file_attrs.source_path, file_attrs.source_path_len, 0);
	send(server_socket, file_attrs.destination_path, file_attrs.destination_path_len, 0);
	
	if ((receive_message(&server_socket, &message_flag, message)) != EXIT_SUCCESS) {
		fprintf(stderr, "Something went wrong. Socket was closed.\n");
		return EXIT_FAILURE;
	}
	if (message_flag != ACK_OK) {
		printf("%s\n", message);
		close(server_socket);
		exit(EXIT_FAILURE);
	}
	
	close(server_socket);
	
	return EXIT_SUCCESS;
}
