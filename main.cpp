#include "WebSocket.h"
#include "ChatService.h"

#include <iostream>

//#define LETS_DO_TWO

int main(int argc, const char * argv[])
{
    ChatService serviceOne;
    WebSocket one(serviceOne, 1975);

#ifdef LETS_DO_TWO
    ChatService serviceTwo;
    WebSocket two(serviceTwo, 1976);
#endif

    one.join();
#ifdef LETS_DO_TWO
    two.join();
#endif
    
    return 0;
}

