#include <string>
#include <vector>
#include <mutex>
#include <switch.h>
#pragma once

class Logger{
private:
    Mutex printLock;
    int fd;
public:
    Logger();
    ~Logger();
    void print(std::string text);
    void printBuffer(std::vector<u8> buffer);
};

extern Logger g_Logger;