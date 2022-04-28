#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "parson.h"
#include "requests.h"
#include "parson.h"

char* sign_up() {
	char username[LINELEN];
	char password[LINELEN];
	char data[BUFLEN];

	printf("username=");
	scanf("%s", username);
	printf("password=");
	scanf("%s", password);

	sprintf(data, "{\n"
				"\"username\": \"%s\",\n"
				"\"password\": \"%s\"\n"
				"}", username, password);

	return compute_post_request(HOST_IP, "/api/v1/tema/auth/register", CONTENT_TYPE, data, NULL);
}

char* sign_in() {
	char username[LINELEN];
	char password[LINELEN];
	char data[BUFLEN];

	printf("username=");
	scanf("%s", username);
	printf("password=");
	scanf("%s", password);

	sprintf(data, "{\n"
				"\"username\": \"%s\",\n"
				"\"password\": \"%s\"\n"
				"}", username, password);

	return compute_post_request(HOST_IP, "/api/v1/tema/auth/login", CONTENT_TYPE, data, NULL);
}

char* add_book(char* token) {
	char title[LINELEN];
	char author[LINELEN];
	char genre[LINELEN];
	char publisher[LINELEN];
	int page_count;
	char book[BUFLEN];

	printf("title=");
	scanf("%s", title);
	printf("author=");
	scanf("%s", author);
	printf("genre=");
	scanf("%s", genre);
	printf("publisher=");
	scanf("%s", publisher);
	printf("page_count=");
	scanf("%d", &page_count);

	sprintf(book, "{\n"
				  "\"title\": \"%s\",\n"
				  "\"author\": \"%s\",\n"
				  "\"genre\": \"%s\",\n"
				  "\"publisher\": \"%s\",\n"
				  "\"page_count\": \"%d\"\n"
				  "}", title, author, genre, publisher, page_count);

	return compute_post_request(HOST_IP, "/api/v1/tema/library/books", CONTENT_TYPE, book, token);

}


int main(){
	int sockfd;
	char *message;
	char *response;
	char *cookie;
	char *token = NULL;
	int portno = 8080;
	char command[LINELEN];
	memset(command, 0, LINELEN);

	while (strcmp(command, "exit")) {
		scanf("%s", command);
		sockfd = open_connection(HOST_IP, portno, AF_INET, SOCK_STREAM, 0);
		if (strcmp(command, "register") == 0) {
			message = sign_up();
			send_to_server(sockfd, message);
	 		response = receive_from_server(sockfd);
	 		if(strstr(response, "Bad Request"))
	 			printf("%s\n", strstr(response, "{\"error"));
	 		else
	 			printf("Successful!\n");
		} else if (strcmp(command, "login") == 0) {
			message = sign_in();
			send_to_server(sockfd, message);
	 		response = receive_from_server(sockfd);
	 		if(strstr(response, "Bad Request"))
	 			printf("%s\n", strstr(response, "{\"error"));
	 		else {
	 			printf("You're now logged in!\n");
	 			char* cookie_start = strstr(response, "connect.sid");
	 			cookie = strtok(cookie_start, ";");
	 		}
		} else if (strcmp(command, "enter_library") == 0) {
			message = compute_get_request(HOST_IP, "/api/v1/tema/library/access", NULL, cookie);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			token = strstr(response, ":\"");
			token = strtok(token, "\"");
			token = strtok(NULL, "\"");
			if(strstr(response, "Unauthorized"))
	 			printf("{\"error\":\"You are not logged in!\"}\n");
	 		else
	 			printf("You're now in the library!\n");
		} else if (strcmp(command, "get_books") == 0) {
			message = compute_get_request(HOST_IP, "/api/v1/tema/library/books", token, NULL);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			if (token == NULL) {
				printf("%s\n", strstr(response, "{\"error"));
			}
			else {
				printf("%s\n", strstr(response, "[{"));
			}
		} else if (strcmp(command, "get_book") == 0) {
			int id;
			char url[BUFLEN];
			printf("id=");
			scanf("%d", &id);
			sprintf(url, "/api/v1/tema/library/books/%d", id);
			message = compute_get_request(HOST_IP, url, token, NULL);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			if (strstr(response, "{\"error")) {
				printf("%s\n", strstr(response, "{\"error"));
			} else {
				printf("%s\n", strstr(response, "[{"));
			}
		} else if (strcmp(command, "add_book") == 0) {
			message = add_book(token);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			if (strstr(response, "{\"error")) {
				printf("%s\n", strstr(response, "{\"error"));
			} else {
				printf("Book added!\n");
			}
		} else if (strcmp(command, "delete") == 0) {
			int id;
			char url[BUFLEN];
			printf("id=");
			scanf("%d", &id);
			sprintf(url, "/api/v1/tema/library/books/%d", id);
			message = compute_delete_request(HOST_IP, url, token);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			if (strstr(response, "{\"error")) {
				printf("%s\n", strstr(response, "{\"error"));
			} else {
				printf("Book id: %d deleted!\n", id);
			}
		} else if (strcmp(command, "logout") == 0) {
			message = compute_get_request(HOST_IP, "/api/v1/tema/auth/logout", NULL, cookie);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			if (strstr(response, "{\"error")) {
				printf("%s\n", strstr(response, "{\"error"));
			} else {
				printf("Logout successful!\n");	
				token = NULL;
				cookie = NULL;
			}
		} else {
			printf("Unknown command!\n");
		}

		close(sockfd);
	}
}