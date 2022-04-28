#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <netinet/tcp.h>

// Parseaza continutul unui pachet de la un client UDP
void parse_UDP(UdpMessage packet) {
	char topic[LEN_TOPIC];
	int type;
	strncpy(topic, packet.message, LEN_TOPIC - 1);
	topic[LEN_TOPIC - 1] = '\0';
	type = packet.message[50];

	if (type == 0) {
		int sign = packet.message[51];
		uint32_t value;
		memcpy(&value, packet.message + 52, 4);
		value = htonl(value);

		if(sign)
			value *= -1;
		printf("%s:%d - %s - INT - %d\n", packet.ip, packet.port, topic, value);
	} else if(type == 1) {
		uint16_t value;
		memcpy(&value, packet.message + 51, 2);
		value = htons(value);
		printf("%s:%d - %s - SHORT_REAL - %hu.%02d\n", packet.ip, 
						packet.port, topic, (uint16_t)value / 100, value % 100);
	} else if(type == 2) {
		int sign = packet.message[51];
		uint8_t power;
		uint32_t value;
		float final;
		int divider = 1;
		memcpy(&value, packet.message + 52, 4);
		value = htonl(value);
		memcpy(&power, packet.message + 56, 1);

		for(int i = 0; i < power; i++)
			divider *= 10;
		final = (float)value / divider;

		if(sign)
			final *= -1;

		printf("%s:%d - %s - FLOAT - %f\n", packet.ip, packet.port, topic, final);
	} else if(type == 3) {
		char content[1501];
		memcpy(&content, packet.message + 51, 1500);
		content[1500] = '\0';
		printf("%s:%d - %s - STRING - %s\n", packet.ip, packet.port, topic, content);
	}
}

int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	int off =  1;

	if (argc < 3) {
		return -1;
	}
	// se deschide socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");
	// socketul este legat la server
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &off, sizeof(int));
	DIE(ret < 0, "nodelay");
	send(sockfd, argv[1], strlen(argv[1]), 0);

	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(STDIN_FILENO, &read_set);
	FD_SET(sockfd, &read_set);

	int fd_max = STDIN_FILENO > sockfd ? STDIN_FILENO : sockfd;
	
	while (1) {
		fd_set tmp_set = read_set;
		select(fd_max + 1, &tmp_set, NULL, NULL, NULL);
		// primeste o comanda de la tastatura si o evalueaza
		if(FD_ISSET(STDIN_FILENO, &tmp_set)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			if (strncmp(buffer, "exit", 4) == 0) {
				send(sockfd, buffer, sizeof(buffer), 0);
				break;
			}
			if (strstr(buffer, "unsubscribe")){
				char topic_name[51];
				sscanf(buffer, "unsubscribe %s\n", topic_name);
				topic_name[51] = '\0';
				printf("Unsubscribed from topic.\n");
			}
			else if (strstr(buffer, "subscribe")){
				char topic_name[51];
				int sf;
				sscanf(buffer, "subscribe %s %d\n", topic_name, &sf);
				topic_name[51] = '\0';
				printf("Subscribed to topic.\n");
			} else
				continue;

			n = send(sockfd, buffer, strlen(buffer), 0);
			DIE(n < 0, "send");

		} else if(FD_ISSET(sockfd, &tmp_set)) {
			/* daca primeste pachet pe socket, acesta poate fi doar raspunsul 
			serverului la incercarea de conectare sau un mesaj de la un client
			udp */
			UdpMessage topic_update;
			n = recv(sockfd, &topic_update, sizeof(UdpMessage), 0);
			DIE(n < 0, "recv");
			if(strcmp(topic_update.message, "exit") == 0)
				break;
			parse_UDP(topic_update);

		}
	}

	close(sockfd);

	return 0;
}
