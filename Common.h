//
// Created by ironzhou on 2020/6/11.
//
#pragma once
#ifndef ECHO_COMMON_H
#define ECHO_COMMON_H
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#define ServerIP "0.0.0.0"
#define ServerPort 4096
#define BufferSize 2048
struct echo_data {
    char* data;
    int fd;
};
#endif //ECHO_COMMON_H
