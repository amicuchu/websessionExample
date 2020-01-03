#include <vector>
#include <queue>
#include <switch.h>
#include "Broadcast.hpp"

class WebSession{
private:
        AppletHolder* appletHolder;
        Thread thread;
        std::queue<std::vector<u8>> messagesToSend;
        std::size_t lastMessageStorageSize;
        UEvent messageOnQueueEvent;
        Broadcast<std::vector<u8>> messageReceivedBroadcast;
        u32 theWhateverValue;

        static void staticThreadExec(void *arg);
        void threadExec();
        void calibrateWhateverValue();
        void processIncomingMessage();
        void processOutcomingMessage();
        void ackIncomingMessage(size_t storageSize);

public:
        WebSession(AppletHolder* holder);
        ~WebSession();

        void sendMessage(const std::vector<u8> data);
        void startThread();
        void waitForThread();
};