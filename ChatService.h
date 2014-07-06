#ifndef LibWebSocketExample__ChatService
#define LibWebSocketExample__ChatService

#include "WebSocket.h"

#include <iostream>
#include <set>
#include <thread>
#include <deque>

class ChatService : public WebSocket::Delegate {
public:
    ChatService();
private:
    void run();
    void newClient(unsigned int id);
    void message(unsigned int id, const std::string& msg);
    void clientGone(unsigned int id);

    std::set<unsigned int> clients;
    std::mutex queueLock;
    std::deque<std::string> queue;
    std::condition_variable condition;
    bool running;
    std::thread worker;
};


#endif 