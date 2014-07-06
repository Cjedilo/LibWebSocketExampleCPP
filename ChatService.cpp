#include "ChatService.h"


ChatService::ChatService() :
    running(true),
    worker(&ChatService::run, this)
{
}

void ChatService::newClient(unsigned int id)
{
    std::cout << "new one: " << id << std::endl;

    clients.insert(id);
}

void ChatService::message(unsigned int id, const std::string& msg)
{
    std::cout << id << " says: " << msg << std::endl;
    {
        std::lock_guard<std::mutex> lock(queueLock);
        queue.push_back(msg);
        condition.notify_one();
    }
}

void ChatService::clientGone(unsigned int id)
{
    std::cout << "That was: " << id << std::endl;

    clients.erase(id);
}

void ChatService::run()
{
    std::mutex                   signal;
    std::unique_lock<std::mutex> lock(signal);

    while(running) {

        while(queue.empty() && running)
        {
            condition.wait(lock);
        }

        if(!running) return;

        std::deque<std::string> toSend;
        {
            std::lock_guard<std::mutex> lock(queueLock);
            std::swap(toSend, queue);
        }

        for(auto i : clients) {
            for(auto& msg : toSend) {
                std::cout << "Telling: " << msg << " to " << i << std::endl;

                send(i,msg);
            }
        }
    }
}