#include "Log.hpp"
#include <switch.h>
#include <iostream>
#include <cstdio>

Logger::Logger(){
    socketInitializeDefault();
    mutexInit(&printLock);
    fd = nxlinkStdio();
}

Logger::~Logger(){
    if(fd>0) close(fd);
}

void Logger::print(std::string text){
    mutexLock(&printLock);
    std::cout << text << "\n";
    consoleUpdate(NULL);
    mutexUnlock(&printLock);
}

void Logger::printBuffer(std::vector<u8> buffer){
    mutexLock(&printLock);
    for(u8 num : buffer){
        printf("0x%x ", num);
    }
    std::cout << "\n";
    consoleUpdate(NULL);
    mutexUnlock(&printLock);
}