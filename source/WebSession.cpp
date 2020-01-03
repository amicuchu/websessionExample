#include "WebSession.hpp"
#include "Log.hpp"
#include <cstring>

struct PACKED SessionMessageHeader{
    unsigned int messageID;
    u32 size;
    u64 pad;
};

struct PACKED AckData{
    u32 ackedSize;
    u32 whatever;
    u32 reserved;
};

void fillPopInteractiveOutDataEvent(AppletHolder *holder){
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 1;

    while(R_FAILED(rc)){
        serviceAssumeDomain(&holder->s);
        rc = serviceDispatch(&holder->s, 106,
            .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
            .out_handles = &tmp_handle,
        );
        if(R_FAILED(rc)) svcSleepThread(5000000);
    }
    
    eventLoadRemote(&holder->PopInteractiveOutDataEvent, tmp_handle, false);
    
}

WebSession::WebSession(AppletHolder* holder){
    this->appletHolder = holder;
    Result rc = threadCreate(&this->thread, &WebSession::staticThreadExec, this, nullptr, 0x10000, 0x2C, -2);
    g_Logger.print("Thread created");
    if(R_FAILED(rc)){
        throw rc;
    }

    ueventCreate(&this->messageOnQueueEvent, false);
}

WebSession::~WebSession(){
    threadClose(&this->thread);
}

void WebSession::sendMessage(const std::vector<u8> data){
    this->messagesToSend.push(data);
}

void WebSession::startThread(){
    Result rc = threadStart(&this->thread);
    g_Logger.print("Thread started");
    if(R_FAILED(rc)){
        throw rc;
    }
}

void WebSession::waitForThread(){
    Result rc = threadWaitForExit(&this->thread);
    if(R_FAILED(rc)){
        g_Logger.print("Thread wait failed: " + rc);
        throw rc;
    }
}

void WebSession::staticThreadExec(void* arg){
    auto webSession = reinterpret_cast<WebSession*>(arg);
    //g_Logger.print("Static thread trampolin");
    try{
        webSession->threadExec();
    }catch(Result rc){
        g_Logger.print("NX error code: " + std::to_string(rc));
    }catch(...){
        g_Logger.print("Unknown exception");
    }
}

void WebSession::threadExec(){
    svcSleepThread(5000000000);
    g_Logger.print("Web session thread");
    fillPopInteractiveOutDataEvent(appletHolder);
    g_Logger.print("PopInteractiveOutDataEvent getted");

    calibrateWhateverValue();
    g_Logger.print("Whatever value obtained");

    while(true){
        s32 selectedWaiter;
        Result rc;

        if(this->lastMessageStorageSize == 0){
            const Waiter waiters[] = {
                waiterForEvent(&this->appletHolder->PopInteractiveOutDataEvent),
                waiterForEvent(&this->appletHolder->StateChangedEvent),
                waiterForUEvent(&this->messageOnQueueEvent)
            };
            rc = waitObjects(&selectedWaiter, waiters, sizeof(waiters) / sizeof(Waiter), U64_MAX);
        }else{
            const Waiter waiters[] = {
                waiterForEvent(&this->appletHolder->PopInteractiveOutDataEvent),
                waiterForEvent(&this->appletHolder->StateChangedEvent)
            };
            rc = waitObjects(&selectedWaiter, waiters, sizeof(waiters) / sizeof(Waiter), U64_MAX);
        }
        
        if(rc == KERNELRESULT(TimedOut)){
            continue;
        }else if(R_FAILED(rc)){
            throw rc;
        }

        switch (selectedWaiter) {
        case 0:
            g_Logger.print("WebSession: Data incoming");
            //Interactive data avaiable
            processIncomingMessage();
            break;
        case 1:
            g_Logger.print("WebSession: AppletHolder status changed");
            //Applet holder state changed
            return;
        case 2:
            g_Logger.print("WebSession: Sending message");
            //Message on queue
            processOutcomingMessage();
            break;
        default:
            break;
        }
    }
}

void WebSession::calibrateWhateverValue(){
    AppletStorage storage;
    {
        g_Logger.print("Calibration send message");
        char string[] = "SYNC";
        std::vector<u8> message(sizeof(SessionMessageHeader) + sizeof(string));

        auto header = reinterpret_cast<SessionMessageHeader*>(message.data());
        header->messageID = 0x0;
        header->pad = 0;
        header->size = sizeof(string);

        char *data = reinterpret_cast<char*>(message.data() + sizeof(SessionMessageHeader));
        memcpy(data, &string, sizeof(string));

        Result rc = appletCreateStorage(&storage, message.size());
        if(R_FAILED(rc)){
            throw rc;
        }

        rc = appletStorageWrite(&storage, 0, message.data(), message.size());
        if(R_FAILED(rc)){
            appletStorageClose(&storage);
            throw rc;
        }

        rc = appletHolderPushInteractiveInData(appletHolder, &storage);
        if(R_FAILED(rc)){
            appletStorageClose(&storage);
            throw rc;
        }
    }
    
    g_Logger.print("Calibration send ended");

    {
        appletHolderWaitInteractiveOut(appletHolder);

        g_Logger.print("Calibration receive");

        Result rc = appletHolderPopInteractiveOutData(this->appletHolder, &storage);
        if(R_FAILED(rc)){
            throw rc;
        }

        s64 size;
        rc = appletStorageGetSize(&storage, &size);
        if(R_FAILED(rc)){
            appletStorageClose(&storage);
            throw rc;
        }

        std::vector<u8> message(size);
        rc = appletStorageRead(&storage, 0, message.data(), size);
        if(R_FAILED(rc)){
            appletStorageClose(&storage);
            throw rc;
        }

        g_Logger.print("Ack buffer:");
        g_Logger.printBuffer(message);

        AckData* data = reinterpret_cast<AckData*>(message.data() + sizeof(struct SessionMessageHeader));
        theWhateverValue = data->whatever;
        g_Logger.print("The whatever value is " + std::to_string(theWhateverValue));
    }
    
}

void WebSession::processIncomingMessage(){
    AppletStorage storage;
    Result rc = appletHolderPopInteractiveOutData(this->appletHolder, &storage);
    if(R_FAILED(rc)){
        throw rc;
    }

    s64 size;
    rc = appletStorageGetSize(&storage, &size);
    if(R_FAILED(rc)){
        appletStorageClose(&storage);
        throw rc;
    }

    std::vector<u8> buffer(size);
    appletStorageRead(&storage, 0, buffer.data(), size);
    if(R_FAILED(rc)){
        appletStorageClose(&storage);
        throw rc;
    }

    struct SessionMessageHeader* message = reinterpret_cast<struct SessionMessageHeader*>(buffer.data());
    if(message->messageID == 0x1000){
        //Ack
        struct AckData* ack = reinterpret_cast<struct AckData*>(buffer.data() + sizeof(struct SessionMessageHeader));
        if(ack->ackedSize == this->lastMessageStorageSize){
            this->lastMessageStorageSize = 0;
            processOutcomingMessage();
        }else{
            appletStorageClose(&storage);
            throw "Ack mismatch";
        }
    }else if(message->messageID == 0){
        //Message to receive
        std::vector<u8> data(buffer.cbegin() + sizeof(struct SessionMessageHeader), buffer.cend());
        std::string dataDec(reinterpret_cast<char*>(data.data()), data.size() - 1);
        g_Logger.print(dataDec);

        this->messageReceivedBroadcast.broadcast(data);
        ackIncomingMessage(buffer.size());
    }

    appletStorageClose(&storage);
}

void WebSession::ackIncomingMessage(size_t storageSize){
    g_Logger.print("WebSession: Ack data received");
    //Disable this part because crashes
    std::vector<u8> message(sizeof(struct SessionMessageHeader) + sizeof(struct AckData) + 4);
    
    SessionMessageHeader* header = reinterpret_cast<SessionMessageHeader*>(message.data());
    header->messageID = 0x1000;
    header->pad = 0;
    header->size = sizeof(struct AckData);

    AckData* data = reinterpret_cast<AckData*>(message.data() + sizeof(struct SessionMessageHeader));
    data->ackedSize = storageSize;
    data->reserved = 0;
    data->whatever = theWhateverValue;
    *(message.end() - 1) = 0;
    *(message.end() - 2) = 0;
    *(message.end() - 3) = 0;
    *(message.end() - 4) = 0;

    AppletStorage storage;
    Result rc;

    rc = appletCreateStorage(&storage, message.size());
    if(R_FAILED(rc)){
        throw rc;
    }

    rc = appletStorageWrite(&storage, 0, message.data(), message.size());
    if(R_FAILED(rc)){
        appletStorageClose(&storage);
        throw rc;
    }

    rc = appletHolderPushInteractiveInData(appletHolder, &storage);
    if(R_FAILED(rc)){
        appletStorageClose(&storage);
        throw rc;
    }
}

void WebSession::processOutcomingMessage(){
    if(!this->messagesToSend.empty()){
        const std::vector<u8> buffer = this->messagesToSend.front();
        this->messagesToSend.pop();

        AppletStorage storage;
        Result rc = appletCreateStorage(&storage, buffer.size());
        if(R_FAILED(rc)){
            throw rc;
        }

        rc = appletPushInteractiveOutData(&storage);
        if(R_FAILED(rc)){
            appletStorageClose(&storage);
            throw rc;
        }
        this->lastMessageStorageSize = buffer.size();
    }else{
        ueventClear(&this->messageOnQueueEvent);
    }
}