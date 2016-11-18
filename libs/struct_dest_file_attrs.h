#ifndef _STRUCT_HEADER_H_
#define _STRUCT_HEADER_H_

struct dest_file_attrs {
	unsigned char structure_len;
	char command;
	long int path_len;
	unsigned long long data_len;
	unsigned long long discard;
	
} dest_file_attrs;

#endif /*!__STRUCT_HEADER_H__*/
