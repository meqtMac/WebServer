//
//  main.cpp
//  WebServer
//
//  Created by 蒋艺 on 2022/11/28.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "locker.hpp"
#include "threadpool.hpp"
#include <signal.h>
#include "http_conn.hpp"
#include <exception>


#define MAX_FD 65535 // max file describer num
#define MAX_EVENT_NUMBER 10000 // max event number
/**
 * @brief add sigaction
 * @param sig signal to be catch
 * @param handler signal handler
*/
void addsig(int sig, void(handler)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

extern void addfd(int epollfd, int fd, bool oneshot);
extern void removefd(int epollfd, int fd);
extern void modifyfd(int epollfd, int fd, int event);

int main(int argc, const char * argv[]) {
    // insert code here...
    if (argc < 1) {
        printf("please input according to format: %s port_number\n",  basename(argv[0]));
        exit(-1);
    }

    // get port
    int port = atoi(argv[1]);

    // Add signal hander
    addsig(SIGPIPE, SIG_IGN); // ignore SIGPIPE

    // Create threadpoll
    Threadpool<HttpConn>* pool = NULL;
    try {
        pool = new Threadpool<HttpConn>;
    } catch (...) {
        return 1;
    }

    // Array to save connection info
    HttpConn * users = new HttpConn[MAX_FD];

    // Create TCP Connection socket and return it's file describer
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        throw std::exception();
        exit(-1);
    }

    // Port multiplexing, must be set before bind
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Bind
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = ntohs(port);
    int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address) );
    if (ret==-1) {
        throw std::exception();
    }

    // Listen
    listen(listenfd, 5);

    // Create epoll object, event array, 
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);

    // add fd to epoll
    addfd(epollfd, listenfd, false);
    HttpConn::m_epollfd = epollfd;

    while(true) {
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ( (num<0) and (errno != EINTR) ) {
            printf("epoll failure\n");
            break;
        }

        // Iteator event arrary
        for (int i=0; i<num; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd==listenfd) {
                // new client connection
                struct sockaddr_in clientAddr;
                socklen_t clientAddressLength = sizeof(clientAddr);
                int connectfd = accept(listenfd, (struct sockaddr*)&clientAddr, &clientAddressLength);
                if (connectfd < 0) {
                    printf("errno is : %d\n", errno);
                    continue;
                }

                if (HttpConn::m_user_count >= MAX_FD) {
                    // connection number overflow
                    //TODO: error reply
                    close(connectfd);
                    continue;
                }

                // init connection
                users[connectfd].init(connectfd, clientAddr);
                
            }else if( events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR) ) {
                // error 
                users[sockfd].closeConnection();
            }else if ( events[i].events & EPOLLIN ) {
                // received data
                if ( users[sockfd].read() ) {
                    pool->append(users+sockfd);
                }else{
                    // read failed
                    users[sockfd].closeConnection();
                }
            }else if ( events[i].events & EPOLLOUT ) {
                if ( !users[sockfd].write() ) {
                    // write failed 
                    users[sockfd].closeConnection();
                }
            }
        }

    }
    close(epollfd);
    close(listenfd);
    delete [] users;
    delete pool;
    return 0;
}
