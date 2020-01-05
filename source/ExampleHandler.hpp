#include "WebSessionHandler.hpp"

class ExampleHandler : public WebSessionHandler{
public:
    void giveWebSession(std::weak_ptr<WebSession> webSession) override;
    void handleMessage(const std::vector<u8> &data) override;
private:
    std::weak_ptr<WebSession> webSession;
};