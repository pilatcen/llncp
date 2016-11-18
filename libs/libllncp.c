#include "libllncp.h"


/* ########################################################################################################## */
/* ***		_strnlen: because solaris still doesn't have strnlen										  *** */
/* ########################################################################################################## */

size_t _strnlen(const char *s, size_t max)
{
    register const char *p;
    
    p = s;
    while ((*p) && max) {
		max--;
		p++;
	}
    return(p - s);
}

/* ########################################################################################################## */
/* ***		print_buffer: debug function																  *** */
/* ########################################################################################################## */

void print_buffer(char *buffer, int byte_count, int type)
{
	int i;
	for (i=0; i<byte_count;i++){
		if (type == 1) {
			printf("%c|", buffer[i]);
		} else {
			printf("%d|", buffer[i]);
		}
	}
	printf("\n");
}

/* ########################################################################################################## */
/* ***		recv_bytes: receive certain number of bytes													  *** */
/* ########################################################################################################## */

inline int recv_bytes(char *buffer, int *socket, unsigned long long bytes)
{
	unsigned long long n, received_bytes = 0;
	
	while (received_bytes < bytes) {
		n = recv(*socket, buffer + received_bytes, bytes - received_bytes, 0);
		if (n == 0){
			return SOCKET_CLOSED;
		}else if (n < 0) {
			return EXIT_FAILURE;
		}
		
		received_bytes += n;
	}
	return EXIT_SUCCESS;
}

/* ########################################################################################################## */
/* ***		send_bytes: send bytes to non-blocking socket												  *** */
/* ########################################################################################################## */

inline int send_bytes(char *buffer, int *write_socket, unsigned long long bytes)
{
	unsigned long long n;
	int retval;
	fd_set write_set;
	unsigned long long sent_bytes = 0;
	
	while (sent_bytes < bytes) {
		FD_ZERO(&write_set);
		FD_SET(*write_socket, &write_set);
		
		retval = select (*write_socket+1, NULL, &write_set, NULL, NULL);
		if (FD_ISSET(*write_socket, &write_set)) {
			n = send(*write_socket, buffer + sent_bytes, bytes - sent_bytes, 0);
			if ((n < 1) && (errno == EWOULDBLOCK)) {
				continue;
			} else if (n == -1) {
				perror("FATAL ERROR send");
				return SOCKET_CLOSED;
			}
			
			sent_bytes += n;
		} else if (retval == -1) {
			perror("FATAL ERROR select");
			return EXIT_FAILURE;
		}
	}
	
	return EXIT_SUCCESS;
}

/* ########################################################################################################## */
/* ***		send_path: send file path																	  *** */
/* ########################################################################################################## */

inline int send_path(char *path, int *write_socket, long int *len)
{
	long int remaining_bytes = *len % (BUFFERSIZE - 1);
	long int i;
	int error;
	
	for (i = 0; i < (*len / (BUFFERSIZE - 1)); i++) {
		if ((error = send_bytes(path + (BUFFERSIZE - 1) * i, write_socket, BUFFERSIZE)) != EXIT_SUCCESS) {
			return error;
		}
	}
	
	if (remaining_bytes) {
		if ((error = send_bytes(path + (*len - remaining_bytes), write_socket, remaining_bytes)) != EXIT_SUCCESS) {
			return error;
		}
	}
	return EXIT_SUCCESS;
}

/* ########################################################################################################## */
/* ***		load_path: load file path																	  *** */
/* ########################################################################################################## */

inline int load_path(long int *path_len, int *socket, char *path)
{
	int error;
	
	if (*path_len > PATH_MAX) {
		return FILE_ERROR;
	}
	
	if ((error = recv_bytes(path, socket, *path_len)) != EXIT_SUCCESS) {
		return error;
	}
	
	return EXIT_SUCCESS;
}

/* ###################################################################################################### */
/* ***		send_message: send message between server, client_daemon, client.                         *** */
/* ***		Used for authentication, and acknowledges for successful/unsuccessful file transfer.	  *** */
/* ###################################################################################################### */

inline int send_message(int *socket, char message_flag, char *message)
{
	int message_len;
	char message_buf[MESSAGE_BUFFERSIZE];
	
	message_len = _strnlen(message, MESSAGE_BUFFERSIZE);
	
	if (message_len == 0) {
		message_len +=2;
	} else {
		memcpy(message_buf + 2, message, message_len + 1);
		message_len +=3;
	}
	
	message_buf[0] = (unsigned char) message_len;
	message_buf[1] = message_flag;
	
	
	
	if (send(*socket, message_buf, message_len, 0) == -1) {
		return errno;
	}
	
	return EXIT_SUCCESS;
}

/* ###################################################################################################### */
/* ***		receive_message													                          *** */
/* ###################################################################################################### */

inline int receive_message(int *socket, char *message_flag, char *message)
{
	int error;
	char header[2];
	int message_len;
	
	header[0] = 0;
	header[1] = ACK_ERROR;
	
	if ((error = recv_bytes(header, socket, 2)) != EXIT_SUCCESS) {
		return error;
	}
	
	*message_flag = header[1];
	message_len = (unsigned char)header[0];
	
	if (message_len == 0) {
		return(SOCKET_CLOSED);
	}
	
	if (((message_len - 2) > 0) && ((error = recv_bytes(message, socket, message_len - 2)) != EXIT_SUCCESS)) {
		return error;
	}
	
	return EXIT_SUCCESS;
}


