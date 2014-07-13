#ifndef LibWebSocketExample__WebSocket
#define LibWebSocketExample__WebSocket

#include <libwebsockets.h>

#include <thread>
#include <mutex>
#include <map>
#include <queue>

class WebSocket {
public:
    class Delegate {
    public:
        virtual void newClient(unsigned int id) =0;
        virtual void message(unsigned int id, const std::string& msg) =0;
        virtual void clientGone(unsigned int id) =0;
        void send(unsigned int id, const std::string& msg);
        void setSocket(WebSocket* socket);
    private:
        WebSocket* socket;
    };
    WebSocket(Delegate& delegate, unsigned short port);
    ~WebSocket();
    void join();

    void newClient(int id,libwebsocket *wsi);
    void message(int id, const std::string& msg);
    void clientGone(int id);
    void send(unsigned int id, const std::string& msg);
    std::queue<std::string> fetchMessageQueue(unsigned int id);
private:
    void service();
    bool init();
private:
    std::mutex queueLock;
    std::map<unsigned int, std::queue<std::string>> messageQueue;
    std::map<unsigned int, libwebsocket*> clients;
    Delegate& delegate;
    unsigned short port;
    lws_context_creation_info socketInfo;
    libwebsocket_context *context;
    bool shouldService;
    std::thread serviceThread;
};

#endif
