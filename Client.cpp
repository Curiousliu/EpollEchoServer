//
// Created by ironzhou on 2020/6/11.
//
#include <iostream>
#include "Common.h"

using namespace std;

int main(int argc,char** argv) {
    //Create socket
    struct sockaddr_in serverAddr;
    char buf[BufferSize];
    int clientFd = socket(AF_INET, SOCK_STREAM, 0);//tcp
    if(clientFd < 0){
        cout<<"Client Create Socket Error:"<<strerror(errno)<<endl;
        exit(-1);
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5000);
    inet_pton(AF_INET, ServerIP, &serverAddr.sin_addr);

    //Connect
    if( connect(clientFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0 ){
        close(clientFd);
        cout<<"Connect Error:"<<strerror(errno)<<endl;
        exit(-1);
    }

    //Echo
    while(true){
        memset(buf, 0, sizeof(buf));
        cin>>buf;
        write(clientFd, buf, sizeof(buf));
        memset(buf, 0, sizeof(buf));
        read(clientFd, buf, sizeof(buf));
        cout<<buf<<endl;
    }

    //Close
    close(clientFd);
    return EXIT_SUCCESS;
}
