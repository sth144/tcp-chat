/**************************************************************************
 *  Title: Chat Client Class Specification
 *  Author: Sean Hinds
 *  Date: 25 Oct, 2018
 *  Description: Specification for chat client class which implements a 
 *  			client in a TCP chat application
 * ************************************************************************/

#include <iostream>		
#include <cstdlib>		
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>			// socket header
#include <sys/types.h>			// used for some socket API calls
#include <netdb.h>

using namespace std;

class ChatClient {
private:

    /* private data members */
    bool connected;
    int sock_fd,
        buffer_size,
        max_handle_length;
    char* server_host,
        * server_port,
        * server_handle,
        * client_handle;
    unsigned char* buffer_in,
                 * buffer_out; 
    struct addrinfo* server_results,
                   * server;

    /* messages */
    const char* quit_msg = "\\quit";
    const char* request_handle_msg = "SEND_HANDLE";
    const char* null_msg = "\0"; 
   
    /* private methods */
    char* getClientHandle();
    struct addrinfo* lookupServerAddress(char*, char*, struct addrinfo**);
    int createSocket(struct addrinfo**, struct addrinfo**);
    bool connectToServer(int, struct addrinfo*);
    void sendClientHandle(int, unsigned char*);
    char* getServerHandle(int, unsigned char*, unsigned char*, int);
    void chatLoop(bool*, int, unsigned char*, unsigned char*);
    unsigned char* getClientMessage(int, unsigned char*);
    void promptClient(char*);
    unsigned char* getServerMessage(int, unsigned char*);
    void displayServerMessage(char*, unsigned char*);
    void clientDisconnect(int);
    void handleServerDisconnect(int);
    void stop(int);
    void cleanup();

public:
    /* public methods */
    ChatClient(char*, char*, int);
    void startup();
    void sigint_intercept();
    ~ChatClient();
};
