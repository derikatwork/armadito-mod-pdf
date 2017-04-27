#ifndef _PDF_ERRORS_H
#define _PDF_ERRORS_H
#endif


/* PDF library errors definitions */
#define ERROR_SUCCESS 0

// Loading errors flags
#define ERROR_ON_LOAD (1 << 4)
#define ERROR_INVALID_FD			1 		/* (invalid file handle or file descriptor) */
#define ERROR_FILE_NOT_FOUND		2
#define ERROR_ZERO_LENGTH_FILE		3
#define ERROR_INSUFFICENT_MEMORY	4
#define ERROR_FILE_READ				5
#define ERROR_INVALID_FORMAT		6