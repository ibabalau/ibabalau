extern "C"{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
}

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
using namespace std;

#define MAX_CLIENTS 60000
#define BUFLEN 1700

//accepts tcp client and puts it into fd_set
int accept_client(int sockfd_tcp, struct sockaddr_in &cli_addr, fd_set &read_fds, int &fdmax) {
    socklen_t clilen = sizeof(cli_addr);
    int newsockfd_tcp = accept(sockfd_tcp, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd_tcp < 0){
        fprintf(stderr, "tcp accept err\n");
        exit(-1);
    }
    int enable = 1;
    if (setsockopt(newsockfd_tcp, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    FD_SET(newsockfd_tcp, &read_fds);
    if (newsockfd_tcp > fdmax) {
        fdmax = newsockfd_tcp;
    }
    return newsockfd_tcp;
}

//extract topic from buffer
string get_topic(char *buffer, int start, int end) {
    string topic = "";
    for (int i = start; i < end; i++) {
        if (buffer[i] == ' ' || buffer[i] == '\0' || buffer[i] == '\n')
            break;
        topic += buffer[i];
    }
    return topic;
}

//recieve udp message and send it to subscribed tcp clients
void recv_udp(int sockfd_udp,
                map<string, set<string>> sf_clients,
                map<string, vector<string>> &sf_messages,
                fd_set set,
                char *buffer,
                map<string, vector<string>> &topics,
                map<string, int> &id_to_sock) {

    struct sockaddr_in cli_addr;
    socklen_t len = sizeof(cli_addr);
    int n = recvfrom(sockfd_udp, (char *)buffer, BUFLEN,
        0, (struct sockaddr*) &cli_addr, &len);

    string topic = get_topic(buffer, 0, 50);
    if (!topics[topic].empty()) {
        //put udp client ip and port into buffer
        char temp[50];
        buffer[n++] = '\0';
        sprintf(temp, "%s:", inet_ntoa(cli_addr.sin_addr));
        for (int i = n; i < n + (int)strlen(temp); i++)
            buffer[i] = temp[i-n];
        n += strlen(temp);
        sprintf(temp, "%d", ntohs(cli_addr.sin_port));
        for (int i = n; i < n + (int)strlen(temp); i++)
            buffer[i] = temp[i-n];
        n += strlen(temp);
        buffer[n++] = '\0';

        //send buffer to subscribed tcp clients, or put it into sf buffer if clients are offline
        for (vector<string>::iterator it = topics[topic].begin() ; it != topics[topic].end(); ++it) {
            if (FD_ISSET(id_to_sock[*it], &set)) {
                send(id_to_sock[*it], buffer, BUFLEN, 0);
            }
            else
               if (sf_clients[topic].find(*it) != sf_clients[topic].end()){
                    string message = "";
                    for (int i = 0; i < n; i++)
                        message += buffer[i];
                    sf_messages[*it].push_back(message);
               }
        }
    }
}

//parse TCP client messagess
void parse_client_comm(char *buffer,
                        int n,
                        int sock_fd,
                        map<string, vector<string>> &topics,
                        map<int, string> sock_to_id,
                        map<string, set<string>> &sf_clients) {
    if (strstr(buffer, "unsubscribe") != NULL) {
        string topic = get_topic(buffer, 12, n);
        //check if client is in unsubscribed vector for the topic
        vector<string>::iterator position = find(topics[topic].begin(),
            topics[topic].end(), sock_to_id[sock_fd]);
        if (position != topics[topic].end())
            topics[topic].erase(position);
        else {
            cout << "Client " << sock_to_id[sock_fd] << "is not subscribed to topic " << topic << '\n';
        }
        //remove client from sf vector
        if (sf_clients[topic].find(sock_to_id[sock_fd]) !=  sf_clients[topic].end())
            sf_clients[topic].erase(sf_clients[topic].find(sock_to_id[sock_fd]));
    }

    else if (strstr(buffer, "subscribe") != NULL) {
        string topic = "";
        int j;
        //get topic
        for (j = 10; j < n; j++){
            if (buffer[j] == ' ' || buffer[j] == '\0')
                break;
            else
                topic += buffer[j];
        }
        j++;
        //check if subscribe command is valid
        if (buffer[j] != '1' && buffer[j] != '0')
            return;
        vector<string>::iterator position = find(topics[topic].begin(),
            topics[topic].end(), sock_to_id[sock_fd]);
        //insert client ID into topic subscribe vector
        if (position == topics[topic].end()) {
            topics[topic].push_back(sock_to_id[sock_fd]);
            //if clients has SF 1, insert into sf vector as well
            if (buffer[j] == '1')
                sf_clients[topic].insert(sock_to_id[sock_fd]);
        }
        else {
            cout << "Client " << sock_to_id[sock_fd] << "is already subscribed to topic " << topic << '\n';
        }
    }
}

//initialize sockets and fd sets
void init_sockets(int &sockfd_tcp, int &sockfd_udp, fd_set &read_fds, int port, int &fdmax) {
    int ret;
    struct sockaddr_in serv_addr;
    int enable = 1;
    //tcp socket
    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_tcp < 0) {
        printf("socket tcp error\n");
        exit(-1);
    }
    //udp socket
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_udp < 0) {
        printf("socket udp error\n");
        exit(-1);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    //set socket options
    if (setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    if (setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    if (setsockopt(sockfd_udp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    ret = bind(sockfd_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
    if (ret < 0){
        fprintf(stderr, "tcp bind err\n");
        exit(-1);
    }

    ret = bind(sockfd_udp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
    if (ret < 0){
        fprintf(stderr, "udp bind err\n");
        exit(-1);
    }

    ret = listen(sockfd_tcp, MAX_CLIENTS);
    if (ret < 0){
        fprintf(stderr, "tcp listen err\n");
        exit(-1);
    }

    FD_SET(sockfd_tcp, &read_fds);
    FD_SET(sockfd_udp, &read_fds);
    fdmax = sockfd_tcp >= sockfd_udp ? sockfd_tcp : sockfd_udp;
    FD_SET(STDIN_FILENO, &read_fds);
    if(STDIN_FILENO > fdmax){
        fdmax = STDIN_FILENO;
    }
}

int main(int argc, char *argv[]) {
    int sockfd_tcp, sockfd_udp, newsockfd_tcp, port;
    char buffer[BUFLEN];
    struct sockaddr_in cli_addr;
    int n, i, ret, fdmax;
    //maps socket id to client id
    map<int, string> sock_to_id;
    //maps client_id to socket id
    map<string, int> id_to_sock;
    //maps each topic with a vector of tcp clients subscribed to it
    map<string, vector<string>> topics;
    //maps each topic, to a set of tcp clients, that have SF for it
    map<string, set<string>> sf_clients;
    //maps a tcp client, with all the messages he missed while offline
    map<string, vector<string>> sf_messages;

    fd_set read_fds;
    fd_set tmp_fds;

    if (argc < 2) {
        exit(-1);
    }

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    port = atoi(argv[1]);
    if (port == 0){
        printf("atoi error\n");
        exit(-1);
    }

    init_sockets(sockfd_tcp, sockfd_udp, read_fds, port, fdmax);

    while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        if (ret < 0) {
            fprintf(stderr, "select err\n");
            exit(-1);
        }

        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == sockfd_tcp) {
                    //accept new client
                    newsockfd_tcp = accept_client(sockfd_tcp, cli_addr, read_fds, fdmax);
                    memset(buffer, 0, BUFLEN);
                    n = recv(newsockfd_tcp, buffer, sizeof(buffer), 0);
                    string id = "";
                    printf("New client %s connected from %s:%d.\n",
                        buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                    for (int j = 0; j < n; j++)
                        id += buffer[j];
                    sock_to_id[newsockfd_tcp] = id;
                    id_to_sock[id] = newsockfd_tcp;

                    //check if client got any messages while he was offline
                    if (!sf_messages[id].empty()) {
                        //send them
                        for (int j = 0; j < (int)sf_messages[id].size(); j++) {
                            send(newsockfd_tcp, sf_messages[id].at(j).c_str(), BUFLEN, 0);
                        }
                        //clear the stored messages
                        sf_messages[id].clear();
                    }
                }
                else
                    if (i == sockfd_udp) {
                        recv_udp(sockfd_udp, sf_clients, sf_messages, read_fds, buffer, topics, id_to_sock);
                    }
                    else
                        if(i == STDIN_FILENO) {
                            //check exit command
                            memset(buffer, 0, BUFLEN);
                            fgets(buffer, BUFLEN - 1, stdin);
                            if (strncmp(buffer, "exit", 4) == 0) {
                                for (int j = 0; j <= fdmax; j++) {
                                    if (FD_ISSET(j, &read_fds) && j != sockfd_tcp
                                        && j != STDIN_FILENO && j != sockfd_udp)
                                        send(j, buffer, sizeof(buffer), 0);
                                }
                                close(sockfd_tcp);
                                close(sockfd_udp);
                                return 0;
                            }
                        }
                        else {
                            //recieved message from TCP client
                            memset(buffer, 0, BUFLEN);
                            n = recv(i, buffer, sizeof(buffer), 0);
                            if (n < 0) {
                                fprintf(stderr, "tcp recv %d err\n", __LINE__);
                                exit(-1);
                            }
                            if (n == 0) {
                                //client closed connection
                                cout << "Client " << sock_to_id[i] << " disconnected.\n";
                                close(i);
                                FD_CLR(i, &read_fds);
                            }
                            else {
                                parse_client_comm(buffer, n, i, topics, sock_to_id, sf_clients);
                            }
                        }
            }
        }
    }
    close(sockfd_tcp);
    return 0;
}