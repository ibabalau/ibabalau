extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
}

#include <iostream>
#include <iomanip>

using namespace std;

#define MAX_CLIENTS 60000
#define BUFLEN 1700
#define ID_LEN 10

string get_ip_port(char *buffer, int start, int end) {
    string ip_port = "";
    for (int i = start; i < end; i++) {
        if (buffer[i] == '\0')
            break;
        else
            ip_port += buffer[i];
    }
    return ip_port;
}

//parse message recieved from server
void parse_print(char *buffer, int n) {
    string ip_port = "";
    string topic = "";
    for (int i = 0; i < 50; i++) {
        if (buffer[i] == ' ' || buffer[i] == '\0')
            break;
        else
            topic += buffer[i];
    }
    if ((int) buffer[50] == 0) {
        int value = 0;
        value = (unsigned char) buffer[52];
        value = (value << 8) + ((unsigned char) buffer[53]);
        value = (value << 8) + ((unsigned char) buffer[54]);
        value = (value << 8) + ((unsigned char) buffer[55]);
        ip_port = get_ip_port(buffer, 57, n);
        if ((int) buffer[51] == 1)
            value *= -1;
        string message = ip_port + " - " + topic + " - " + "INT" + " - ";
        cout << message << value << '\n';
    }
    else
        if ((int) buffer[50] == 1) {
            uint16_t value = 0;
            value = (unsigned char) buffer[51];
            value = (value << 8) + ((unsigned char) buffer[52]);
            ip_port = get_ip_port(buffer, 54, n);
            string message = ip_port + " - " + topic + " - " + "SHORT_REAL" + " - ";
            cout << message << (float)value / 100 << '\n';
        }
        else
            if ((int) buffer[50] == 2) {
                int value = 0;
                value = (unsigned char) buffer[52];
                value = (value << 8) + ((unsigned char) buffer[53]);
                value = (value << 8) + ((unsigned char) buffer[54]);
                value = (value << 8) + ((unsigned char) buffer[55]);
                uint8_t value2 = 0;
                value2 = (unsigned char) buffer[56];
                float float_value = (float) value;
                for(int i = 0; i < value2; i++)
                    float_value /= 10;
                if ((int) buffer[51] == 1)
                    float_value *= -1;
                ip_port = get_ip_port(buffer, 58, n);
                int digits = 0;
                int temp = value;
                //get number of digits
                while (temp != 0){
                    temp /= 10;
                    digits++;
                }
                string message = ip_port + " - " + topic + " - " + "FLOAT" + " - ";
                cout << message << setprecision(digits) << float_value << '\n';
            }
            else
                if ((int) buffer[50] == 3) {
                    int j;
                    string str = "";
                    for (j = 51; j < n; j++) {
                        if (buffer[j] == '\0')
                            break;
                        else
                            str += buffer[j];
                    }
                    ip_port = get_ip_port(buffer, j+1, n);
                    string message = ip_port + " - " + topic + " - " + "STRING" + " - " + str;
                    cout << message << '\n';
                }
}

int main(int argc, char *argv[]) {
    int sockfd, n, ret;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];
    //buffer pentru id client
    char id[ID_LEN];

    //init sets
    fd_set read_fds;
    fd_set tmp_fds;
    int fdmax;
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(-1);
    }

    strcpy(id, argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    ret = inet_aton(argv[2], &serv_addr.sin_addr);
    if (ret == 0) {
        perror("inet_aton");
        exit(-1);
    }
    int enable = 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        fprintf(stderr, "connect error\n");
        exit(-1);
    }
    memset(buffer, 0, BUFLEN);
    strcpy(buffer, id);
    n = send(sockfd, buffer, strlen(buffer), 0);
    FD_SET(sockfd, &read_fds);
    fdmax = sockfd;
    FD_SET(STDIN_FILENO, &read_fds);
    if (STDIN_FILENO > fdmax){
        fdmax = STDIN_FILENO;
    }

    while (1) {
        tmp_fds = read_fds;
        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        if (ret < 0) {
            fprintf(stderr, "select error\n");
            exit(-1);
        }
        for (int i = 0; i <= fdmax; i++){
            if (FD_ISSET(i, &tmp_fds)) {
                //recieved message from server
                if (i == sockfd) {
                    memset(buffer, 0, BUFLEN);
                    n = recv(i, buffer,sizeof(buffer),0);
                    if (n < 0) {
                        fprintf(stderr, "recv error\n");
                        exit(-1);
                    }
                    //check if server sent exit commmand
                    if (strncmp(buffer, "exit", 4) == 0) {
                        close(sockfd);
                        return 0;
                    }
                    else
                        parse_print(buffer, n);
                }
                //recieved keyboard input
                else if(i == STDIN_FILENO) {
                    memset(buffer, 0, BUFLEN);
                    fgets(buffer, BUFLEN - 1, stdin);
                    if (strncmp(buffer, "exit", 4) == 0) {
                        close(sockfd);
                        return 0;
                    }
                    //send message to server
                    n = send(sockfd, buffer, strlen(buffer), 0);
                    if (strstr(buffer, "unsubscribe") != NULL) {
                        //get topic and print
                        string topic = "";
                        for (int i = 12; i < n; i++){
                            if (buffer[i] == ' ' || buffer[i] == '\0' || buffer[i] == '\n')
                                break;
                            else
                                topic += buffer[i];
                        }
                        cout << "unsubscribed " << topic << '\n';
                    }
                    else if (strstr(buffer, "subscribe") != NULL) {
                        string topic = "";
                        //check if input is valid
                        for (int i = 10; i < n; i++) {
                            if (buffer[i] == ' ') {
                                if (buffer[i+1] != '0' && buffer[i+1] != '1'){
                                    printf("Invalid SF argument. Stop.\n");
                                    break;
                                }
                                cout << "subscribed " << topic << '\n';
                                break;
                            }
                            else if (buffer[i] == '\0' || buffer[i] == '\n') {
                                printf("No SF argument. Stop.\n");
                                break;
                            }
                            else
                                topic += buffer[i];
                        }
                    }
                    else
                        printf("Command not found\n");
                }
            }
        }
    }
    close(sockfd);
    return 0;
}
