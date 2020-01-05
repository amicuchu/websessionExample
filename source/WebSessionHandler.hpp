#pragma once
#include <vector>
#include <memory>
#include <switch.h>
#include "WebSession.hpp"

class WebSession;

class WebSessionHandler{
public:
    virtual void giveWebSession(std::weak_ptr<WebSession> webSession) = 0;
    virtual void handleMessage(const std::vector<u8> &data) = 0;
};