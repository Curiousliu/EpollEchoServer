//
// Created by ironzhou on 2020/6/12.
//
#include <iostream>
#include <sys/types.h>
#include <sys/epoll.h>
#include "Common.h"
#define ListenBacklog SOMAXCONN
#define EpollSize 60000
using namespace std;

void addFd(int epollFd, int fd){
    struct epoll_event epollEvent;
    epollEvent.data.fd = fd;
    epollEvent.events = EPOLLIN | EPOLLET; //use ET mode; multiple threads use EPOLLONESHOT
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &epollEvent) != 0){// is this necessary?
        close(fd);close(epollFd);
        cout<<"Epoll CTL Error:"<<strerror(errno)<<endl;
        exit(-1);
    }
    if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK) != 0){//is this "if" necessary?
        //set non-blocking I/O
        close(fd);close(epollFd);
        cout<<"Setting I/O Error:"<<strerror(errno)<<endl;
        exit(-1);
    }
}

void EventHandle(int epfd, struct epoll_event ev){
    int fd = ev.data.fd;
    if(ev.events & EPOLLIN){
        char* buf = (char*)malloc(BufferSize);
        memset(buf, 0, BufferSize);
        int count = 0;
        int n = 0;
        //repeatly read
        while(1){
            n = read(fd, (buf+count), 10);
            if(n > 0){
                count += n;
            }
            else if(n == 0){
                break;
            }
            else if(n < 0 && errno == EAGAIN){
                //cout<<strerror(errno)<<endl;
                break;
            }
            else{
                cout<<"Read Error:"<<strerror(errno)<<endl;
                exit(-1);
            }
        }
        if(count == 0){
            if(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev) != 0){
                cout<<"Epoll CTL Delete Error:"<<strerror(errno)<<endl;
                exit(-1);
            }
            close(fd);
            return;
        }

        //set to write
        struct echo_data* ed = (struct echo_data*)malloc(sizeof(struct echo_data));
        ed->data = buf;
        ed->fd = fd;
        ev.data.ptr = ed;
        ev.events = EPOLLOUT | EPOLLET;
        if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) != 0) {
            cout<<"Epoll Read-Write state Changing Error:"<<strerror(errno)<<endl;
            exit(-1);
        }
        return;
    }
    else if(ev.events & EPOLLOUT){
        struct echo_data* data = (struct echo_data*)ev.data.ptr;
        int ret = 0;
        int send_pos = 0;
        const int total = strlen(data->data);
        //string send_buf = data->data;
        char* send_buf = data->data;
        while(1) {
            ret = write(data->fd, (send_buf + send_pos), total - send_pos);
            if (ret < 0) {
                if (errno == EAGAIN) {
                    sched_yield();
                    continue;
                }
                cout<<"Write Error:"<<strerror(errno)<<endl;
                exit(-1);
            }
            send_pos += ret;
            if (total == send_pos) {
                break;
            }
        }

        //set to read
        ev.data.fd = data->fd;
        ev.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epfd, EPOLL_CTL_MOD, data->fd, &ev) != 0) {
            cout<<"Epoll Write-Read state Changing Error:"<<strerror(errno)<<endl;
            exit(-1);
        }

        free(data->data);
        free(data);
    }

    return;
}


int main(int argc, char** argv){
    //Create socket
    int serverFd, clientFd, epollFd;
    struct sockaddr_in serverAddr, clientAddr;
    struct epoll_event epollEvent, epollEvents[EpollSize];
    //char recvBuff[BufferSize], sendBuff[BufferSize];
    char buff[BufferSize];
    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(ServerPort);
    inet_pton(AF_INET, ServerIP, &serverAddr.sin_addr);//change IP(Decimal to binary)
    serverFd = socket(AF_INET, SOCK_STREAM, 0);//tcp
    if(serverFd < 0){
        close(serverFd);
        cout<<"Server Socket Error:"<<strerror(errno)<<endl;
        exit(-1);
    }

    //Bind
    if(bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0){
        close(serverFd);
        cout<<"Server Binding Rrror:"<<strerror(errno)<<endl;
        exit(-1);
    }

    //Listen
    if(listen(serverFd, ListenBacklog) != 0){//how to decide backlog? in linux the maximum is 128
        close(serverFd);
        cout<<"Listening Error:"<<strerror(errno)<<endl;
        exit(-1);
    }

    //Create epoll
    //epollFd = epoll_create(EpollSize);//Linux 2.6.8 later, size ignored
    epollFd = epoll_create1(EPOLL_CLOEXEC);//or 0
    if(epollFd < 0){
        close(serverFd);close(epollFd);
        cout<<"Epoll Creating Error:"<<strerror(errno)<<endl;
        exit(-1);
    }
    addFd(epollFd, serverFd);


    //Main
    while(1){
        int EpollEventNumber = epoll_wait(epollFd, epollEvents, EpollSize, -1);//-1:block/ 0:non-block
        if(EpollEventNumber < 0){
            cout<<"Epoll Waiting Error"<<strerror(errno)<<endl;
            exit(-1);//continue break? exit?
        }
        for(int i = 0; i < EpollEventNumber ; i++){
            //Accept
            if(epollEvents[i].data.fd == serverFd){//new connection
                socklen_t clientAddrLen = sizeof(struct sockaddr_in);
                //multiple connections at the same time
                while((clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen)) > 0){
                    addFd(epollFd, clientFd);
                    cout<<"Client IP addr:("<<inet_ntoa(clientAddr.sin_addr)<<":"
                    <<ntohs(clientAddr.sin_port)<<"), ClientFd："<<clientFd<<endl;
                }
                if(clientFd < 0){
                    if(errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR){
                        cout<<"Accept Error"<<endl;
                    }
                }
            }
            else{
                EventHandle(epollFd, epollEvents[i]);
            }

        }

    }

    //Close
    close(serverFd);close(epollFd);close(clientFd);
    return EXIT_SUCCESS;
}


