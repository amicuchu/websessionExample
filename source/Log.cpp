#include "Log.hpp"
#include <switch.h>
#include <iostream>
#include <cstdio>

Logger::Logger(){
    socketInitializeDefault();
    mutexInit(&printLock);
    //fd = nxlinkStdio();
}

Logger::~Logger(){
    //if(fd>0) close(fd);
}

void Logger::print(std::string text){
    mutexLock(&printLock);
    std::cout << text << "\n";
    consoleUpdate(NULL);
    mutexUnlock(&printLock);
}