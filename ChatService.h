#ifndef LibWebSocketExample__ChatService
#define LibWebSocketExample__ChatService

#include "WebSocket.h"

#include <iostream>
#include <set>
#include <thread>
#include <deque>
#include <condition_variable>

class ChatService : public WebSocket::Delegate {
public:
    ChatService();
    ~ChatService();
    void run();
    void newClient(unsigned int id);
    void message(unsigned int id, const std::string& msg);
    void clientGone(unsigned int id);

private:
    std::mutex clientLock;
    std::set<unsigned int> clients;
    std::mutex queueLock;
    std::deque<std::string> queue;
    std::condition_variable condition;
    bool running;
    std::thread worker;
};


#endif 
