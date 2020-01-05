#pragma once
#include <vector>
#include <list>
#include <queue>
#include <memory>
#include <switch.h>
#include <memory>
#include "WebSessionHandler.hpp"

class WebSessionHandler;

class WebSession{
private:
        AppletHolder* appletHolder;
        Thread thread;
        std::queue<std::vector<u8>> messagesToSend;
        std::size_t lastMessageStorageSize;
        UEvent messageOnQueueEvent;
        std::list<std::unique_ptr<WebSessionHandler>> handlers;
        std::shared_ptr<WebSession> masterPointer;
        u32 theWhateverValue;

        static void staticThreadExec(void *arg);
        void threadExec();
        void calibrateWhateverValue();
        void processIncomingMessage();
        void processOutcomingMessage();
        void ackIncomingMessage(u32 storageSize);

public:
        WebSession(AppletHolder* holder);
        ~WebSession();

        void sendMessage(const std::vector<u8> &&data);
        void sendMessage(const std::vector<u8> &data);
        void addHandler(std::unique_ptr<WebSessionHandler> handler);
        void startThread();
        void waitForThread();
};