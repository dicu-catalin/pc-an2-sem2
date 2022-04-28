#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <stdbool.h>

// verifica daca un client este activ
bool active_client(Client *clients, int nr_clients, char *id) {
	for (int i = 0; i < nr_clients; i++){
		if (strcmp(clients[i].id, id) == 0)
			return true;
	}
	return false;
}

// Intoarce id-ul clientului care are un anuit socket
char *find_client_ID(Client *clients, int nr_clients, int socket) {
	for (int i = 0; i < nr_clients; i++)
		if (clients[i].socket == socket)
			return clients[i].id;
	return NULL;
}
// Intoarce pozitia unui client care s-a deconectat
int find_client(OldClient *clients, int nr_clients, char *id) {
	for (int i = 0; i < nr_clients; i++) {
		if (strcmp(clients[i].id, id) == 0)
			return i;
	}
	return -1;
}

// Aboneaza/dezaboneaza un client de la un anumit topic
void tcp_client(int socket, Client *clients, int nr_clients, 
							char *buffer, Topic *topics, int *curr_topics){
	char topic_name[LEN_TOPIC];
	if (strstr(buffer, "unsubscribe")) {
		sscanf(buffer, "unsubscribe %s\n", topic_name);
		topic_name[LEN_TOPIC-1] = '\0';
		int i;  // pozitia topicului in vector
		for(i = 0; i < *curr_topics; i++)
			if(strcmp(topic_name, topics[i].name) == 0)
				break;

		for(int j = 0; j < topics[i].nr_clients; j++) 
			if(topics[i].clients[j].socket == socket) {
				topics[i].clients[j] = topics[i].clients[topics[i].nr_clients - 1];
				topics[i].nr_clients--;
			}
	}
	else if (strstr(buffer, "subscribe")) {
		int sf;

		sscanf(buffer, "subscribe %s %d\n", topic_name, &sf);
		topic_name[LEN_TOPIC-1] = '\0';
		int i;
		for(i = 0; i < *curr_topics; i++)
			if(strcmp(topic_name, topics[i].name) == 0)
				break;

		if (i == *curr_topics) {
			strcpy(topics[i].name, topic_name);
			topics[i].clients[0].socket = socket;
			char *client_id = find_client_ID(clients, nr_clients, socket);
			if(client_id)
				strcpy(topics[i].clients[0].id, client_id);
			topics[i].clients[0].type = sf;
			topics[i].nr_clients++;
			(*curr_topics)++;
		} else {
			int cur_clients = topics[i].nr_clients;
			topics[i].clients[cur_clients].socket = socket;
			topics[i].clients[cur_clients].type = sf;
			char *client_id = find_client_ID(clients, nr_clients, socket);
			if(client_id)
				strcpy(topics[i].clients[cur_clients].id, client_id);
			topics[i].nr_clients++;
		}
	}
}

// trimite mesajul de pe un anumit topic tuturor clientilor activi abonati la el
void udp_client (int sockfd, Client *clients, int nr_clients, Topic *topics, 
							int nr_topics, char *buffer, OldClient *olds, int nr_oldClients) {
	char topic_name[LEN_TOPIC];
	int i;
	UdpMessage packet;
	struct sockaddr_in from_station;
    socklen_t adr_len = sizeof(from_station);
	
	memset(buffer, 0, BUFLEN);
	int n = recvfrom(sockfd, buffer, BUFLEN, 0, 
						(struct sockaddr *)&from_station, &adr_len);
	DIE(n < 0, "recv");
	memcpy(packet.message, buffer, BUFLEN);//, strlen(buffer));
	strcpy(packet.ip, inet_ntoa(from_station.sin_addr));
	packet.port = ntohs(from_station.sin_port);
	strncpy(topic_name, buffer, LEN_TOPIC - 1);
	topic_name[LEN_TOPIC-1] = '\0';
	
	for (i = 0; i < nr_topics; i++)
		if (strcmp(topics[i].name, topic_name) == 0) 
			break;

	if (i == nr_topics) 
		return;
	
	for (int j = 0; j < topics[i].nr_clients; j++) {
		int sock_client = topics[i].clients[j].socket;
		if (active_client(clients, nr_clients, topics[i].clients[j].id)) {
			n = send(sock_client, (char *)&packet, sizeof(UdpMessage), 0);
			DIE(n<0, "send UDP");
		} else if (topics[i].clients[j].type == 1) {
			/* daca un client este deconectat si vrea sa primeasca mesajele, 
			acestea sunt salvate la id-ul corespunzator clientului deconectat*/
			int pos = find_client(olds, nr_oldClients, topics[i].clients[j].id);
			if (pos != -1) {
				memcpy(&(olds[pos].packet[olds[pos].count]), &packet, sizeof(packet));
				olds[pos].count++;
			}

		}
	}

}
/*In main sunt deschisi cei doi socketi pentru TCP si UDP. Atunci
cand primeste un nou client pe TCP, verifica daca id-ul lui a mai fost folosit.
Daca id-ul este unic, acestuia i se permite conectarea. Daca vin date de pe
socketul UDP, acesta apeleaza functia udp_client. Daca vin date pe socketii noi,
Se apeleaza functia care trateaza continutul pachetului, tcp_client.
In main se salveaza topicurile si clientii activi.
*/
int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfdTCP, sockfdUDP, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;
	Client *clients = calloc(sizeof(Client), MAX_CLIENTS);
	int nr_clients = 0;
	Topic topics[MAX_TOPICS];
	int curr_topics = 0;
	// sunt salvate id-urile clientilor care s-au deconectat
	OldClient oldClients[MAX_CLIENTS];
	int nr_oldClients = 0;

	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;	

	if (argc < 2) {
		return -1;
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfdTCP = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfdTCP < 0, "socket");
	sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfdUDP < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfdTCP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockfdTCP, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	ret = bind(sockfdUDP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	
	FD_SET(sockfdTCP, &read_fds);
	FD_SET(sockfdUDP, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	fdmax = STDIN_FILENO > sockfdTCP ? STDIN_FILENO : sockfdTCP;
	if (fdmax < sockfdUDP)
		fdmax = sockfdUDP;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			if (strncmp(buffer, "exit", 4) == 0) {
				UdpMessage packet;
				strcpy(packet.message, "exit");

				for (int j = 0; j < nr_clients; j++) {
					n = send(clients[j].socket, (char *)&packet, sizeof(UdpMessage), 0);
					DIE(n<0, "send exit");
				}
				break;
			}
		}

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfdTCP) {
					char client_id[15];
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfdTCP, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");
					FD_SET(newsockfd, &read_fds);
					recv(newsockfd, client_id, 15, 0);

					if (!active_client(clients, nr_clients, client_id)) {
						clients[nr_clients].socket = newsockfd;
						strcpy(clients[nr_clients].id, client_id);
						nr_clients++;
						int pos = find_client(oldClients, nr_oldClients, client_id);
						/* daca un client a mai fost conectat inainte, acesta
						primeste mesajele care au fost trimise in absenta sa.*/
						if(pos != -1) {
							for(int j = 0; j < oldClients[pos].count; j++)
								send(newsockfd, (char *)&(oldClients[pos].packet[j]), 
									sizeof(UdpMessage), 0);
							nr_oldClients--;
							oldClients[pos] = oldClients[nr_oldClients];
						}
					}
					else {
						printf("Client %s already connected.\n", client_id);
						UdpMessage packet;
						strcpy(packet.message, "exit");
						n = send(newsockfd, (char *)&packet, sizeof(UdpMessage), 0);
						DIE(n<0, "send exit");
						FD_CLR(newsockfd, &read_fds);
						break;
					}

					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					printf("New client %s connected from %s:%d.\n", 
							client_id, inet_ntoa(cli_addr.sin_addr), 
											ntohs(cli_addr.sin_port));
				} else if (i == sockfdUDP) {
					udp_client(sockfdUDP, clients, nr_clients, topics, 
										curr_topics, buffer, oldClients, nr_oldClients);
				} else {
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (strstr(buffer, "exit")) {
						printf("Client %s disconnected.\n", 
							find_client_ID(clients, nr_clients, i));
						strcpy(oldClients[nr_oldClients].id, 
							find_client_ID(clients, nr_clients, i));
						oldClients[nr_oldClients].count = 0;
						nr_oldClients++;
						close(i);
						for (int j = 0; j < nr_clients; j++){
							if (clients[j].socket == i) {
								nr_clients--;
								clients[j] = clients[nr_clients];
							}
						}
						FD_CLR(i, &read_fds);
					} else {
						tcp_client(i, clients, nr_clients, 
								buffer, topics, &curr_topics);
					}
				}
			}
		}
	}

	close(sockfdTCP);

	return 0;
}