#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include <string.h>
#include <string>
#include <iostream>
#include "parsers.h"
#include <limits.h>
#include <thread>
#include <mutex>

#define BUF_SIZE 256
#define SLEEP_TIME 30000
void receive(int sock);
void check_connection(int sock);
std::mutex send_mutex;
int room;
std::mutex room_mutex;


int main(int argc, char *argv[]) {
    if(argc != 4) {
        printf("Invalid argument number\n");
        return 1;
    }
    auto input_vector = parseByDelimeter(std::string(argv[1]), ":", 2);
    if(input_vector.size() != 2) {
        perror("Invalid input");
        exit(1);
    }
    int sockfd;
    int portno = 5000;
    std::string ip = "127.0.0.1";
    ip = input_vector[0];
    portno = std::atoi(input_vector[1].c_str());
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];
    server = gethostbyname(ip.c_str());
    
    std::vector<std::string> response_headers(7);
    std::string name = argv[3];
    room_mutex.lock();
    room = std::atoi(argv[2]);
    room_mutex.unlock();
    bool  userConnected = false;
    
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    
    // TODO : create socket and get file descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("Unable to open a socket");
        exit(1);
    }
    int tr = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
        perror("Setsockopt error");
        exit(1);
    }
    // TODO : connect to server with server address which is set above (serv_addr)
    socklen_t servlen = sizeof(serv_addr);
    if(connect(sockfd, (struct sockaddr*) &serv_addr, servlen) == -1) {
        perror("Unable to connect to the server");
        exit(1);
    }
    // TODO : inside this while loop, implement communicating with read/write or send/recv function
    std::thread receiver;
    std::thread connection_checker;
    while (1) {
        /* Headers mapping
         0 - Type
         1 - Who
         2 - Name
         3 - Room
         4 - Receivers
         5 - SleepTime
         6 - Content
         */
        if(userConnected == false) {
            response_headers[0] = "Connect";
            response_headers[1] = "Client";
            response_headers[2] = name;
            room_mutex.lock();
            response_headers[3] = std::to_string(room);
            room_mutex.unlock();
            response_headers[4] = "";
            response_headers[5] = "0";
            response_headers[6] = "";
            userConnected = true;
            receiver = std::thread(receive, sockfd);
            connection_checker = std::thread(check_connection, sockfd);
        }
        else {
            bzero(buffer,BUF_SIZE);
            std::string input;
            std::getline(std::cin,input);
            if(input.size() == 0) {
                printf("Invalid input. Please try again.\n");
                continue;
            }
            if(input[0] == '/') {
                if(input.find("/list") != std::string::npos) {
                    response_headers[0] = "GetUsers";
                    response_headers[1] = "Client";
                    response_headers[2] = name;
                    room_mutex.lock();
                    response_headers[3] = std::to_string(room);
                    room_mutex.unlock();
                    response_headers[4] = "";
                    response_headers[5] = "";
                    response_headers[6] = "";
                }
                else if(input.find("/join") != std::string::npos) {
                    std::vector<std::string> splitted = parseByDelimeter(input, " ", INT_MAX);
                    if(splitted.size() != 2) {
                        printf("Invalid input. Please try again.\n");
                        continue;
                    }
                    response_headers[0] = "ChangeRoom";
                    response_headers[1] = "Client";
                    response_headers[2] = name;
                    room_mutex.lock();
                    response_headers[3] = std::to_string(room);
                    room_mutex.unlock();
                    response_headers[4] = "";
                    response_headers[5] = "";
                    response_headers[6] = splitted[1];
                }
                else if(input.find("/quit") != std::string::npos) {
                    response_headers[0] = "Disconnect";
                    response_headers[1] = "Client";
                    response_headers[2] = name;
                    room_mutex.lock();
                    response_headers[3] = std::to_string(room);
                    room_mutex.unlock();
                    response_headers[4] = "";
                    response_headers[5] = "";
                    response_headers[6] = "";
                }
                else {
                    printf("Invalid input. Please try again.\n");
                    continue;
                }
            }
            else {
                std::vector<std::string> splitted = parseByDelimeter(input, " : ", INT_MAX);
                if(splitted.size() != 2) {
                    printf("Invalid input(1). Please try again.\n");
                    continue;
                }
                std::string contentOfMessage = splitted[1];
                std::vector<std::string> receiverWithTime = parseByDelimeter(splitted[0], ", ", INT_MAX);
                std::vector<std::string> receivers;
                std::vector<std::string> times;
                bool input_valid = true;
                for(auto i = 0; i < receiverWithTime.size(); i++) {
                    splitted = parseByDelimeter(receiverWithTime[i], "#", INT_MAX);
                    if(splitted.size() > 2 || splitted.size() < 1) {
                        printf("Invalid input(2). Please try again.\n");
                        input_valid = false;
                        break;
                    }
                    receivers.push_back(splitted[0]);
                    if(splitted.size() == 2)
                        times.push_back(splitted[1]);
                    else {
                        times.push_back("0");
                    }
                }
                if(!input_valid)
                    continue;
                std::string receiversZipped = zipArray(receivers);
                std::string timesZipped = zipArray(times);
                response_headers[0] = "Message";
                response_headers[1] = "Client";
                response_headers[2] = name;
                room_mutex.lock();
                response_headers[3] = std::to_string(room);
                room_mutex.unlock();
                response_headers[4] = receiversZipped;
                response_headers[5] = timesZipped;
                response_headers[6] = contentOfMessage;
            }
        }
        std::string response = constructMessage(response_headers);
        send_mutex.lock();
        int send_status = (int)send(sockfd, response.c_str(), response.size(), 0);
        if( send_status <= 0) {
            perror("Error on sending. Terminating the connection\n");
            exit(1);
        }
        usleep(SLEEP_TIME);
        send_mutex.unlock();
    }
    return 0;
}

void receive(int sock) {
    char buffer[BUF_SIZE];
    while(1) {
        bzero(buffer,BUF_SIZE);
        int stat = (int)recv(sock, buffer, BUF_SIZE, 0);
        if(stat == 0) {
            perror("Server is down or timeout");
            exit(1);
        }
        else if(stat < 0) {
            perror("Error on receiving");
            exit(1);
        }
        std::string message(buffer);
        std::vector<std::string> headers = parse(message);
        if(headers[0] == "Connect") {
            std::string room = headers[getHeaderIndex("Room")];
            std::string name = headers[getHeaderIndex("Name")];
            std::cout << "Hello " << name << "! " << "This is room #" << room << "\n";
        }
        else if(headers[0] == "Disconnect") {
            std::string room = headers[getHeaderIndex("Room")];
            std::string name = headers[getHeaderIndex("Name")];
            std::cout << "Good Bye " << name << "\n";
            exit(1);
        }
        else if(headers[0] == "GetUsers") {
            std::string room = headers[getHeaderIndex("Room")];
            std::string name = headers[getHeaderIndex("Name")];
            std::vector<std::string> users = parseByDelimeter(headers[getHeaderIndex("Content")], "#", INT_MAX);
            std::cout << "This is list of users in room # " << room << "\n";
            for(auto i = 0; i < users.size(); i++) {
                std::cout << i+1 << ". " << users[i] << "\n";
            }
        }
        else if(headers[0] == "ChangeRoom") {
            std::string oldRoom = headers[getHeaderIndex("Room")];
            std::string newRoom = headers[getHeaderIndex("Content")];
            std::string name = headers[getHeaderIndex("Name")];
            std::string rec_status = headers[getHeaderIndex("Receivers")];  // Both / Join / Left
            if(rec_status == "Both") {
                std::cout << name << " left room #" << oldRoom << "\n";
                std::cout << name << " joined room #" << newRoom << "\n";
                room_mutex.lock();
                room = std::atoi(newRoom.c_str());
                room_mutex.unlock();
            }
            else if(rec_status == "Left") {
                std::cout << name << " left room #" << oldRoom << "\n";
            }
            else if(rec_status == "Join") {
                std::cout << name << " joined room #" << newRoom << "\n";
            }
            
        }
        else if(headers[0] == "Error") {
            std::string error = headers[getHeaderIndex("Content")];
            std::cout << error << "\n";
        }
        else if(headers[0] == "Message") {
            std::string name = headers[getHeaderIndex("Name")];
            std::string content = headers[getHeaderIndex("Content")];
            std::cout << name << ": " << content << "\n";
        }
        else if(headers[0] == "ConnectionCheck") {
            std::vector<std::string> response_headers(7);
            std::string name = headers[getHeaderIndex("Name")];
            std::string room = headers[getHeaderIndex("Room")];
            response_headers[0] = "Connect";
            response_headers[1] = "Client";
            response_headers[2] = name;
            response_headers[3] = room;
            response_headers[4] = "";
            response_headers[5] = "";
            response_headers[6] = "";
            std::string response = constructMessage(response_headers);
            send_mutex.lock();
            int send_status = (int)send(sock, response.c_str(), response.size(), 0);
            if( send_status <= 0) {
                perror("Error on sending. Terminating the connection\n");
                exit(1);
            }
            usleep(SLEEP_TIME);
            send_mutex.unlock();
            
        }
    }
}
void check_connection(int sock) {
    std::vector<std::string> response_headers(7);
    int sleeptime = 100000 * 20; //2.4 seconds
    while (1) {
        usleep(sleeptime);
        response_headers[0] = "ConnectionCheck";
        response_headers[1] = "Client";
        response_headers[2] = "";
        room_mutex.lock();
        response_headers[3] = std::to_string(room);
        room_mutex.unlock();
        response_headers[4] = "";
        response_headers[5] = "";
        response_headers[6] = "";
        std::string response = constructMessage(response_headers);
        send_mutex.lock();
        int send_status = (int)send(sock, response.c_str(), response.size(), 0);
        if( send_status <= 0) {
            perror("Error on sending. Terminating the connection\n");
            exit(1);
        }
        usleep(SLEEP_TIME);
        send_mutex.unlock();
    }

}



