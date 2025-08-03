/* A cleaner way to parse data from Cobalt Strike */
#include <winsock2.h>
#include <stdlib.h>
#include "parse.h"

/* built-in alloc for our data parser */
datap * data_alloc(int size) {
	datap * temp;
	char  * ptr = NULL;

	/* allocate our data thing */
	temp = (datap *)malloc(sizeof(datap));
	if (temp == NULL)
		return NULL;

	/* allocate our memory */
	ptr = (char *)malloc(size);
	if (ptr == NULL) {
		free(temp);
		return NULL;
	}

	/* init our memory to zero */
	memset(ptr, 0, size);

	/* init the structure that will allow us to walk everything */
	data_init(temp, ptr, size);

	/* return our dataparser */
	return temp;
}

/* free our allocated data block */
void data_free(datap * temp) {
	data_cleanup(temp);
	free(temp->original);
	free(temp);     /* always free this last jerk! */
}

void data_cleanup(datap * temp) {
	memset(temp->original, 0, temp->size);
}

/* init our data structure */
void data_init(datap * temp, char * buffer, int length) {
	temp->original = buffer;
	temp->buffer   = buffer;
	temp->length   = length;
	temp->size     = length;
}

/* extract an integer */
unsigned int data_int(datap * d) {
	unsigned int temp;
	unsigned int * ptr;
	bool check = d->length < sizeof(unsigned int);
	if (check)
		return 0;

	ptr  = (unsigned int *)d->buffer;
	temp = ntohl(*ptr);
	d->length -= sizeof(unsigned int);
	d->buffer = d->buffer + sizeof(unsigned int);
	return temp;
}

/* extract a byte! */
unsigned int data_byte(datap * d) {
	char * ptr;

	if (d->length == 0)
		return 0;

	ptr = d->buffer;
	d->buffer = d->buffer + sizeof(char);
	d->length -= sizeof(char);
	return (unsigned int)*ptr;
}

/* extract a short */
unsigned short data_short(datap * d) {
	unsigned int temp;
	unsigned int * ptr;

	if (d->length < sizeof(unsigned short))
		return 0;

	ptr  = (unsigned int *)d->buffer;
	temp = ntohs(*ptr);
	d->buffer = d->buffer + sizeof(unsigned short);
	d->length -= sizeof(unsigned short);
	return temp;
}

/* extract a pointer */
char * data_ptr(datap * d, int length) {
	char * ptr = d->buffer;

	if (d->length < length)
		return NULL;

	d->buffer  = d->buffer + length;
	d->length -= length;
	return ptr;
}

datap_buffer data_extract(datap * d) {
	datap_buffer buffer;
	buffer.length = data_int(d);
	buffer.buffer = data_ptr(d, buffer.length);
	return buffer;
}

int data_length(datap * d) {
	return d->length;
}

char * data_buffer(datap * d) {
	return d->buffer;
}

/* extract a string from this buffer and null terminate it. Thanks! */
int data_string_oneoff(datap * d, char * buffer, int max) {
	if ((d->length + 1) >= max)
		return 0;

	memcpy(buffer, d->buffer, d->length);
	buffer[d->length] = '\0';

	return d->length + 1;
}

char * data_string_oneoff_FREEME(datap * d, int max) {
	char * temp = (char *)malloc(max);
	data_string_oneoff(d, temp, max);
	return temp;
}

/* extract a string [4b: length][...] */
int data_string(datap * d, char * buffer, int max) {
	int slen;
	char * ptr;

	/* grab our string size */
	slen = data_int(d);
	if (slen == 0 || (slen + 1) >= max)
		return 0;

	/* grab a pointer to whateverz */
	ptr  = data_ptr(d, slen);
	if (ptr == NULL)
		return 0;

	/* create a null terminated string in buffer */
	memcpy(buffer, ptr, slen);
	buffer[slen] = '\0';

	return slen + 1;
}

/* grab a NULL terminated length+string from our buffer, ready for immediate use */
char * data_string_asciiz(datap * d) {
	int slen;
	char * ptr;
	
	/* grab our string size */
	slen = data_int(d);
	if (slen == 0)
		return NULL;

	/* grab a pointer to whateverz */
	ptr = data_ptr(d, slen);
	if (ptr == NULL)
		return NULL;

	return ptr;
}

char * data_ptr_extract(datap * d, int * size) {
	datap_buffer buffer = data_extract(d);

	if (size != NULL)
		*size = buffer.length;

	if (buffer.length == 0)
		return NULL;

	return buffer.buffer;
}