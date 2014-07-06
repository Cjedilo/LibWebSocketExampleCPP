#include "WebSocket.h"

#include <iostream>

//#define REQUEST_ON_THREAD_2

#ifndef REQUEST_ON_THREAD_2
#define CANCEL_ON_THREAD_2
#endif

namespace {
    unsigned int unique = 0;
    std::map<libwebsocket_callback_reasons, std::string> readableEnums = {
        {LWS_CALLBACK_ESTABLISHED," LWS_CALLBACK_ESTABLISHED"},
        {LWS_CALLBACK_CLIENT_CONNECTION_ERROR," LWS_CALLBACK_CLIENT_CONNECTION_ERROR"},
        {LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH," LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH"},
        {LWS_CALLBACK_CLIENT_ESTABLISHED," LWS_CALLBACK_CLIENT_ESTABLISHED"},
        {LWS_CALLBACK_CLOSED," LWS_CALLBACK_CLOSED"},
        {LWS_CALLBACK_CLOSED_HTTP," LWS_CALLBACK_CLOSED_HTTP"},
        {LWS_CALLBACK_RECEIVE," LWS_CALLBACK_RECEIVE"},
        {LWS_CALLBACK_CLIENT_RECEIVE," LWS_CALLBACK_CLIENT_RECEIVE"},
        {LWS_CALLBACK_CLIENT_RECEIVE_PONG," LWS_CALLBACK_CLIENT_RECEIVE_PONG"},
        {LWS_CALLBACK_CLIENT_WRITEABLE," LWS_CALLBACK_CLIENT_WRITEABLE"},
        {LWS_CALLBACK_SERVER_WRITEABLE," LWS_CALLBACK_SERVER_WRITEABLE"},
        {LWS_CALLBACK_HTTP," LWS_CALLBACK_HTTP"},
        {LWS_CALLBACK_HTTP_FILE_COMPLETION," LWS_CALLBACK_HTTP_FILE_COMPLETION"},
        {LWS_CALLBACK_HTTP_WRITEABLE," LWS_CALLBACK_HTTP_WRITEABLE"},
        {LWS_CALLBACK_FILTER_NETWORK_CONNECTION," LWS_CALLBACK_FILTER_NETWORK_CONNECTION"},
        {LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION," LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION"},
        {LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS," LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS"},
        {LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS," LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS"},
        {LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION," LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION"},
        {LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER," LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER"},
        {LWS_CALLBACK_CONFIRM_EXTENSION_OKAY," LWS_CALLBACK_CONFIRM_EXTENSION_OKAY"},
        {LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED," LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED"},
        {LWS_CALLBACK_PROTOCOL_INIT," LWS_CALLBACK_PROTOCOL_INIT"},
        {LWS_CALLBACK_PROTOCOL_DESTROY," LWS_CALLBACK_PROTOCOL_DESTROY"},
        {LWS_CALLBACK_WSI_CREATE," LWS_CALLBACK_WSI_CREATE"},
        {LWS_CALLBACK_WSI_DESTROY," LWS_CALLBACK_WSI_DESTROY"},
        {LWS_CALLBACK_GET_THREAD_ID," LWS_CALLBACK_GET_THREAD_ID"},
        {LWS_CALLBACK_ADD_POLL_FD," LWS_CALLBACK_ADD_POLL_FD"},
        {LWS_CALLBACK_DEL_POLL_FD," LWS_CALLBACK_DEL_POLL_FD"},
        {LWS_CALLBACK_CHANGE_MODE_POLL_FD," LWS_CALLBACK_CHANGE_MODE_POLL_FD"},
        {LWS_CALLBACK_LOCK_POLL," LWS_CALLBACK_LOCK_POLL"},
        {LWS_CALLBACK_UNLOCK_POLL," LWS_CALLBACK_UNLOCK_POLL"}
    };

    int callback_http(libwebsocket_context *context,
                      libwebsocket *wsi,
                      libwebsocket_callback_reasons reason, void *data,
                      void *in, size_t len)
    {
        std::cout << readableEnums[reason] << " callback_http" << std::endl;

        if(reason == LWS_CALLBACK_GET_THREAD_ID) {
            return static_cast<int>(std::hash<std::thread::id>()(std::this_thread::get_id()));
        }

        // We serve a static file in this example, no http needed.
        return 0;
    }
    
    int callback_web (libwebsocket_context *context,
                      libwebsocket *wsi,
                      libwebsocket_callback_reasons reason,
                      void *user, void *in, size_t len)
    {
        std::cout << readableEnums[reason] << " callback_web" << std::endl;

        int &id = *reinterpret_cast<int*>(user);

        if(reason == LWS_CALLBACK_ESTABLISHED) {
            id = ++unique;
            WebSocket::instances[context]->newClient(id,wsi);
        }

        if(reason == LWS_CALLBACK_RECEIVE) {
            std::string msg(reinterpret_cast<char*>(in), len);

            WebSocket::instances[context]->message(id,msg);
        }

        if(reason == LWS_CALLBACK_SERVER_WRITEABLE)
        {
            std::queue<std::string> toSend = WebSocket::instances[context]->fetchMessageQueue(id);
            std::cout << "Got callback" << std::endl;

            while (!toSend.empty()) {
                std::string &msg = toSend.front();
                unsigned char *buf = (unsigned char*)std::malloc(LWS_SEND_BUFFER_PRE_PADDING + msg.size() + LWS_SEND_BUFFER_POST_PADDING);
                std::memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, msg.c_str(), msg.size());
                libwebsocket_write(wsi, buf + LWS_SEND_BUFFER_PRE_PADDING, msg.size(), LWS_WRITE_TEXT);
                std::free(buf);
                toSend.pop();
            }

        }

        if(reason == LWS_CALLBACK_CLOSED)
        {
            WebSocket::instances[context]->clientGone(id);
        }
        
        return 0;
    }

    static struct libwebsocket_protocols protocols[] = {
        /* first protocol must always be HTTP handler */
        {
            "http-only",   // name
            callback_http, // callback
            sizeof(int)              // per_session_data_size
        },
        {
            "web-protocol", // protocol name - very important!
            callback_web,   // callback
            sizeof(int)
        },
        {
            NULL, NULL, 0   /* End of list */
        }
    };
    
    lws_context_creation_info makeInfo(unsigned short port)
    {
        lws_context_creation_info info;

        memset(&info, 0, sizeof(info));

        info.port = port;
        info.protocols = protocols;
        info.extensions = libwebsocket_get_internal_extensions();

        info.gid = -1;
        info.uid = -1;

        info.options = 0;

        return info;
    }
}

void WebSocket::WebSocket::Delegate::send(unsigned int id, const std::string &msg)
{
    socket->send(id,msg);
}

void WebSocket::WebSocket::Delegate::setSocket(WebSocket* socket)
{
    this->socket = socket;
}

std::map<libwebsocket_context*,WebSocket*>    WebSocket::instances;

WebSocket::WebSocket(Delegate& delegate, unsigned short port) :
    delegate(delegate),
    port(port),
    socketInfo(makeInfo(port)),
    context(libwebsocket_create_context(&socketInfo)),
    shouldService(true),
    serviceThread(&WebSocket::service, this)
{
    instances[context] = this;
    delegate.setSocket(this);
}

WebSocket::~WebSocket()
{
    shouldService = false;
    libwebsocket_cancel_service(context);
    serviceThread.join();
    libwebsocket_context_destroy(context);
}

void WebSocket::join()
{
    serviceThread.join();
}

void WebSocket::newClient(int id,libwebsocket *wsi)
{
    clients[id] = wsi;
    delegate.newClient(id);
}

void WebSocket::message(int id, const std::string& msg)
{
    delegate.message(id, msg);
}

void WebSocket::clientGone(int id)
{
    delegate.clientGone(id);
    {
        std::lock_guard<std::mutex> lock(queueLock);
        messageQueue.erase(id);
    }
    clients.erase(id);
}

void WebSocket::send(unsigned int id, const std::string& msg)
{
    {
        std::lock_guard<std::mutex> lock(queueLock);
        messageQueue[id].push(msg);
    }
#ifdef REQUEST_ON_THREAD_2
    libwebsocket_callback_on_writable(context, clients[id]);
#endif
#ifdef CANCEL_ON_THREAD_2
    libwebsocket_cancel_service(context);
#endif
}

std::queue<std::string> WebSocket::fetchMessageQueue(unsigned int id)
{
    std::queue<std::string> queue;
    {
        std::lock_guard<std::mutex> lock(queueLock);
        std::swap(queue, messageQueue[id]);
    }

    return queue;
}

void WebSocket::service()
{
    while(shouldService) {
#ifdef CANCEL_ON_THREAD_2
        {
            std::lock_guard<std::mutex> lock(queueLock);
            for(auto socket : messageQueue) {
                if(!socket.second.empty()) {
                    libwebsocket_callback_on_writable(context, clients[socket.first]);
                }
            }
        }
#endif
        libwebsocket_service(context,std::numeric_limits<int>::max());
    }
}