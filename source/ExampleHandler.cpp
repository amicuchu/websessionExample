#include "ExampleHandler.hpp"
#include "Log.hpp"
#include <string>
#include <cctype>

void ExampleHandler::handleMessage(const std::vector<u8> &data){
    std::string text(reinterpret_cast<const char*>(data.data()), data.size() - 1);
    for(char &c : text){
        c = static_cast<char>(toupper(c));
    }
    
    std::vector<u8> sendData(text.cbegin(), text.cend());
    sendData.push_back(0);
    g_Logger.print("ExampleHandler: data processed");
    auto ws = webSession.lock();
    if(ws){
        g_Logger.print("ExampleHandler: sending processed data");
        ws->sendMessage(std::move(sendData));
    }
}

void ExampleHandler::giveWebSession(std::weak_ptr<WebSession> webSession){
    this->webSession = webSession;
}
