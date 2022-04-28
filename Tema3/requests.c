#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *token,
                            char *cookie) {
	char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // write the method name, URL, request params (if any) and protocol type
    sprintf(line, "GET %s HTTP/1.1", url);

    compute_message(message, line);

    // add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // add token JWT
    if (token != NULL) {
    	memset(line, 0, LINELEN);
    	sprintf(line, "Authorization: Bearer %s", token);
    	compute_message(message, line);
    }
    
    // add login cookie
    if (cookie != NULL) {
    	memset(line, 0, LINELEN);
    	sprintf(line, "Cookie: %s", cookie);
    	compute_message(message, line);
    }

    // add final new line
    compute_message(message, "");
    return message;
}

char* compute_post_request(char *host, char *url, char* content_type, char *body_data,
                            char *token) {

	char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // add necessary headers (Content-Type and Content-Length are mandatory)
 	memset(line, 0, LINELEN);
 	sprintf(line, "Content-Type: %s", content_type);
 	compute_message(message, line);

 	int content_length = strlen(body_data);
 	memset(line, 0, LINELEN);
 	sprintf(line, "Content-Length: %d", content_length);
 	compute_message(message, line);

 	// add JWT token
    if (token != NULL) {
    	memset(line, 0, LINELEN);
    	sprintf(line, "Authorization: Bearer %s", token);
    	compute_message(message, line);
    }

    // add new line at end of header
    compute_message(message, "");

    // add the actual payload data
    memset(line, 0, LINELEN);
    compute_message(message, body_data);

    free(line);
    return message;
}

char* compute_delete_request(char *host, char *url, char* token) {
	char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // write the method name, URL and protocol type
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    // add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // add JWT token
    if (token != NULL) {
    	memset(line, 0, LINELEN);
    	sprintf(line, "Authorization: Bearer %s", token);
    	compute_message(message, line);
    }

    // add final new line
    compute_message(message, "");
    return message;
}