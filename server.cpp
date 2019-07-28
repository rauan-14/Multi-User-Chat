#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string.h>

#include "structs.h"
#include "parsers.h"
#include <thread>
#include <mutex>
#include <limits.h>
#define BUF_SIZE 512
#define SLEEP_TIME 30000

void connect_client(int sock);
void disconnect(int sock, int room, bool userAdded);
Database db; 
std::mutex send_mutex;
struct timeval tv;


int main( int argc, char *argv[] ) {
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5000;
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    clilen = sizeof(cli_addr);
    
    // TODO : create socket and get file descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("Unable to open a socket");
        exit(1);
    }
    int tr = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
        perror("Setsockopt");
        exit(1);
    }
    
    // TODO : Now bind the host address using bind() call.
    if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0 ) {
        perror("Error on binding");
        exit(1);
    }
    
    // TODO : start listening for the clients,
    // here process will go in sleep mode and will wait for the incoming connection
    if(listen(sockfd, 5) < 0) {
        perror("Error on listening");
        exit(1);
    }
    // TODO: Accept actual connection from the client
    std::vector<std::thread> t_list;
    while(1) {
        newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, (socklen_t *)&clilen);
        if(newsockfd > 0) {
            std::cout << "Accepted connection on " << newsockfd << std::endl;
            t_list.push_back(std::thread(connect_client, newsockfd));
        }
    }
    return 0;
}


void connect_client(int sock){
    // TODO : inside this while loop, implement communicating with read/write or send/recv function
    char buffer[BUF_SIZE];
    bool userAdded = false;
    int room = -1;
    std::string name = ""; 
    std::vector<std::string> response_headers(7);
    std::string response;

    /**********/
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    /*********/
    
    while (1) {
        bzero(buffer,256);
        int n = (int)recv(sock, buffer, BUF_SIZE, 0);
        std::cout << "Received message on connection " << sock <<"\n";
        if(n <= 0) {
            //If client disconnected or recv error, close the socket and delete the user
            disconnect(sock, room, userAdded);
            std::cout << "Closed connection on " << sock <<"\n";
            return;
        }
        else {
            std::string message(buffer);
            //std::cout << message << std::endl;
            std::vector<std::string> headers = parse(message);
            room = std::atoi(headers[getHeaderIndex("Room")].c_str());
            name = headers[getHeaderIndex("Name")];
            response_headers[0] = "Response";
            response_headers[1] = "Server";
            response_headers[4] = "";
            response_headers[5] = "";
            response_headers[6] = "";
            response_headers[getHeaderIndex("Room")] = std::to_string(room);
            response_headers[getHeaderIndex("Name")] = name;
        
            if(headers[0] == "Connect") {
                response_headers[0] = "Connect";
                response_headers[getHeaderIndex("Room")] = std::to_string(room);
                User newUser = User(name, sock);
                int username_id = db.addUser(room, newUser);
                if(username_id != 1) {
                    name = name + "-" + std::to_string(username_id); 
                }
                response_headers[getHeaderIndex("Name")] = name;
                userAdded = true;
                send_mutex.lock();
                db.sendJoinMessage(sock, room);
                send_mutex.unlock();
            }
            else {
                if(headers[0] == "Disconnect") {
                    /* /quit message */
                    send_mutex.lock();
                    //db.sendLeaveMessage(sock, room);
                    usleep(SLEEP_TIME);
                    send_mutex.unlock();
                    disconnect(sock, room, userAdded);
                    return;
                }
                else if(headers[0] == "GetUsers") {
                    std::vector<User> users = db.getUserList(room);
                    std::vector<std::string> usernames;
                    for(auto user:users) {
                        usernames.push_back(user.name);
                    }
                    response_headers[0] = "GetUsers";
                    response_headers[getHeaderIndex("Content")] = zipArray(usernames);
                }
                else if (headers[0] == "ChangeRoom") {
                    response_headers[0] = "ChangeRoom";
                    int newRoom = std::atoi(headers[getHeaderIndex("Content")].c_str());
                    send_mutex.lock();
                    db.changeRoom(room, newRoom, sock);
                    usleep(SLEEP_TIME);
                    send_mutex.unlock();
                    response_headers[getHeaderIndex("Content")] = std::to_string(newRoom);
                }
                else if (headers[0] == "Message") {
                    std::vector<std::string> receivers = parseByDelimeter(headers[getHeaderIndex("Receivers")], "#", INT_MAX);
                    std::vector<std::string> times = parseByDelimeter(headers[getHeaderIndex("SleepTime")], "#", INT_MAX);
                    std::vector<int> times_int;
                    for(auto i = 0; i < times.size(); i++) {
                        times_int.push_back(std::atoi(times[i].c_str())); 
                    }
                    if(receivers[0] == "all" || receivers[0] == "All") {
                        send_mutex.lock();
                        db.sendMessageToAll(sock, room, "Message", headers[getHeaderIndex("Content")]);
                        send_mutex.unlock();
                    }
                    else {
                        db.sendMessage(sock, receivers, headers[getHeaderIndex("Content")], room, times_int, &send_mutex);
                    }
 
                }
                else if (headers[0] == "ConnectionCheck") {
                    //Ignore, this type of message is used only for health checking. Look at recv side for health checking
                }
            }
            response = constructMessage(response_headers);
            send_mutex.lock();
            if(send(sock, response.c_str(), response.size(), 0) <= 0) {
                printf("Error on sending.\n");
                send_mutex.unlock();
                return;
            }
            usleep(SLEEP_TIME);
            send_mutex.unlock();
        }
    }
}

void disconnect(int sock, int room, bool userAdded) {
    send_mutex.lock();
    db.sendLeaveMessage(sock, room);
    usleep(SLEEP_TIME);
    send_mutex.unlock();
    if(userAdded) {
        db.deleteUser(room, sock);
    }
    close(sock);
}


