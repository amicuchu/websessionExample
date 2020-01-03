#include <string>
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
};

static Logger g_Logger{};