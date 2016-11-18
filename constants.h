#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

	#define TRUE 1
	#define FALSE 0
	
	#define ACK_OK 1
	#define ACK_ERROR 0
	#define AUTH_REQUEST 2
	
	#define BYTE_SIZE 1
	
	#define QUEUESIZE 1024
	
	#define SOCKET_ADDRESS "/tmp/LLNCP_"
	#define SOCKET_ADDRESS_LEN 11
	
	/* buffer size for transporting data 500*1024 bytes */
	#define BUFFERSIZE 512000
	
	#define MAX_FILE_ATTRS_LEN 1 + 1 + sizeof(long int) + 1 + sizeof(long int) + 1
	#define MIN_FILE_ATTRS_LEN 6
	
	#define MIN_DEST_FILE_ATTRS_LEN 10
	#define MAX_HEADER_LEN 1 + 1 + 1 + 1 + sizeof(long int) + 1 + sizeof(unsigned long long) + 1 + sizeof(unsigned long long) + 1 
	
	#define MESSAGE_BUFFERSIZE 255
	
	#define MK_DIRECTORY 1
	#define MK_FILE 2
	
	/* error constants */
	#define SOCKET_CLOSED 2
	#define PROTOCOL_ERROR 3
	#define FILE_ERROR 4
	#define RECEIVED_BAD_ACK 5
	#define SYSTEM_ERROR 6
	#define BAD_AUTH 7
	#define FORCED_SOCKET_CLOSED 8

#endif /*!__CONSTANTS_H__*/
