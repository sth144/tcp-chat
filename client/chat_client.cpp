/*********************************************************************************************
 *  Title: Chat Client Class Implementation
 *  Author: Sean Hinds
 *  Date: 25 Oct, 2018
 *  Description: Implements the client in a TCP chat application. Connects to a server written
 *  			in python via TCP socket. 
 *  Sources Referenced: https://beej.us/guide/bgnet/html/multi/index.html
 * *******************************************************************************************/

#include "chat_client.hpp"

/* Constructor */

ChatClient::ChatClient(char* server_host, char* server_port, int buffer_size)
{
    this->server_host = server_host;
    this->server_port = server_port;
    this->buffer_size = buffer_size;
    
    buffer_in  = new unsigned char[buffer_size];			// allocate input and
    buffer_out = new unsigned char[buffer_size];			//	output buffers

    server_results = lookupServerAddress(this->server_host, 		// lookup server 
                                         this->server_port, 		//	address via
                                         &server_results);		//	DNS protocol
}

/* start up the client */

void ChatClient::startup()
{
    client_handle = getClientHandle();					// grab user handle input
    sock_fd = createSocket(&server, &server_results);			// create a socket
    if ((connected = connectToServer(sock_fd, server))) 
    {
        sendClientHandle(sock_fd, buffer_out);				// send handle to server
        server_handle = getServerHandle(sock_fd, buffer_in, buffer_out, max_handle_length);
        chatLoop(&connected, sock_fd, buffer_in, buffer_out);		// enter the chat loop
    }
}

/* user inputs a handle */

char* ChatClient::getClientHandle()
{
    char* result = new char[max_handle_length];
    memset(result, 0, max_handle_length);
    bool valid = false;
    int max_length = 10;
    
    while (!valid)
    {
        cout << "Please enter a handle (1-10 characters, no spaces): ";
        valid = true;
        cin >> result;
        if (strlen(result) > max_length)
            valid = false;
        for (int i = 0; i < strlen(result); i++)
        {
            if (result[i] == ' ')
                valid = false;
        }
    }
    return result;
}

/* lookup the server address using DNS and store in a struct addrinfo */

struct addrinfo* ChatClient::lookupServerAddress(char* server_host, 
                                                   char* server_port, 
                                                   struct addrinfo** server_results_ptr)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;			// unspecified family (will be AF_INET for IPv4)
    hints.ai_socktype = SOCK_STREAM;			// TCP socket
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(server_host, server_port, &hints, &server_results) != 0)
    {
        cerr << "Bad address" << endl;
        exit(1);
    }

    return server_results;
}

/* handle opening of the client TCP socket */

int ChatClient::createSocket(struct addrinfo** server_ptr, 
                               struct addrinfo** server_results_ptr)
{
    *server_ptr = *server_results_ptr;
    while (*server_ptr != NULL && ((sock_fd = socket(
    	(*server_ptr)->ai_family, 
    	(*server_ptr)->ai_socktype, 
    	(*server_ptr)->ai_protocol)) == -1))		// iterate over potential server addresses from lookup call
        *server_ptr = (*server_ptr)->ai_next;        
    if (*server_ptr == NULL)				// no suitable server found
    {
        cerr << "Socket failed" << endl;
        exit(1);
    }
    return sock_fd;					// this file descriptor will serve as a handle for the socket
}

/* handle connection of socket to server */

bool ChatClient::connectToServer(int sock_fd, struct addrinfo* server)
{
    int connect_status = -1;
    if ((connect_status = connect(
        sock_fd, 
        server->ai_addr, 
        server->ai_addrlen)) == -1)			// initiate the connection
    {
        cerr << "Connection failed" << endl;
        exit(1);
    }
    return (connect_status >= 0);
}

/* send handle grabbed from user input to server and receive acknowledgement callback */

void ChatClient::sendClientHandle(int sock_fd, unsigned char* buffer_out)
{
    memset(buffer_out, 0, buffer_size);
    memcpy(buffer_out, client_handle, strlen(client_handle));
    send(sock_fd, buffer_out, strlen((char*) buffer_out), 0);
    memset(buffer_in, 0, buffer_size);
    recv(sock_fd, buffer_in, buffer_size, 0);
}

/* request and receive server handle as part of connection setup (not in requirements) */

char* ChatClient::getServerHandle(int sock_fd, 
                                    unsigned char* buffer_in, 
                                    unsigned char* buffer_out,
                                    int max_handle_length)
{
    memset(buffer_out, 0, buffer_size);
    memcpy(buffer_out, request_handle_msg, strlen(request_handle_msg));
    send(sock_fd, buffer_out, strlen((char*) buffer_out), 0);

    memset(buffer_in, 0, buffer_size);
    recv(sock_fd, buffer_in, buffer_size, 0);
    char* ret_handle = new char[max_handle_length];
    memset(ret_handle, 0, max_handle_length);
    memcpy(ret_handle, buffer_in, strlen((char*) buffer_in));

    cout << "Connected to " << buffer_in << endl;
    return ret_handle;
}

/* main chat loop which executes while client is connected to server */

void ChatClient::chatLoop(bool* connected, 
                            int sock_fd,
                            unsigned char* buffer_in, 
                            unsigned char* buffer_out)
{   
    struct timeval tv;				// set up a time struct to dictate
    tv.tv_sec = 0;				// 	how long select() will wait
    tv.tv_usec = 10000;				//	for data on a file descriptor

    fd_set readfds;				// build an fd set
    int stdin_fd = 0;
    bool event = true;

    while (connected) 
    {	
        FD_ZERO(&readfds);			// reset the fd set and rebuild
        FD_SET(stdin_fd, &readfds);
        FD_SET(sock_fd, &readfds);
        if (event) {				// an event has occurred, reprompt client
            promptClient(client_handle);
            event = false;
        }
        select(sock_fd+1, &readfds, NULL, NULL, &tv);	// check fds for data
        if (FD_ISSET(stdin_fd, &readfds)) {		// user has entered a message
            buffer_out = getClientMessage(sock_fd, buffer_out);		// buffer output message
            send(sock_fd, buffer_out, strlen((char*) buffer_out), 0);	// send
            if (strcmp(quit_msg,(char*) buffer_out + strlen(client_handle) + 2) == 0) 
 		clientDisconnect(sock_fd);				// handle client disconneect
            event = true;
        }
        if (FD_ISSET(sock_fd, &readfds)) {		// client has received a message
            if (!FD_ISSET(stdin_fd, &readfds)) {
                cout << "\r";
            }
            buffer_in = getServerMessage(sock_fd, buffer_in);		// buffer server message
            if (strcmp((char*) buffer_in, "\0") == 0) 
                handleServerDisconnect(sock_fd);	// server has disconnected
            displayServerMessage(server_handle, buffer_in);		// display received message
            event = true;
        }
    }
}

/* print prompt with user handle to stdout */

void ChatClient::promptClient(char* client_handle)
{
    if (client_handle)
        cout << client_handle << "> " << flush;
}

/* transfer stdin to output buffer */

unsigned char* ChatClient::getClientMessage(int sock_fd, 
                                     unsigned char* buffer_out)
{
    memset(buffer_out, 0, buffer_size);
    memcpy(buffer_out, client_handle, strlen(client_handle));
    memcpy(buffer_out + strlen(client_handle), "> ", 2);
    cin >> (char*) (buffer_out + strlen(client_handle) + 2);
    return buffer_out;
}

/* transfer data received on the socket to input buffer */

unsigned char* ChatClient::getServerMessage(int sock_fd, 
                                     unsigned char* buffer_in)
{
    memset(buffer_in, 0, buffer_size);
    recv(sock_fd, buffer_in, buffer_size, 0);
    return buffer_in;
}

/* print message from server */

void ChatClient::displayServerMessage(char* server_handle, unsigned char* buffer_in)
{
    cout << buffer_in << endl;
}

/* handle case where client disconnects, prompt to reconnect */

void ChatClient::clientDisconnect(int sock_fd)
{
    cout << client_handle << " disconnected" << endl;
    cout << "Reconnect? (y/n): ";
    char result;
    cin >> result;
    if (result != 'y' && result != 'Y')
        stop(sock_fd);
    else
    {
        delete client_handle;
        startup();
    }
}

/* handle case where server disconnects */

void ChatClient::handleServerDisconnect(int sock_fd)
{	
    cout << server_handle << " terminated the connection" << endl;
    stop(sock_fd);
}

/* stop the client */

void ChatClient::stop(int sock_fd)
{
    connected = false;
    close(sock_fd);
    cleanup();
    exit(1);
}

/* capture SIGINT to shut down gracefully */

void ChatClient::sigint_intercept()
{
    cout << "client disconnecting..." << endl;
    memset(buffer_out, 0, buffer_size);
    memcpy(buffer_out, client_handle, strlen(client_handle));
    memcpy(buffer_out + strlen(client_handle), "> ", 2);
    memcpy(buffer_out + strlen(client_handle) + 2, quit_msg, strlen(quit_msg));
    send(sock_fd, buffer_out, strlen((char*) buffer_out), 0); 
    clientDisconnect(sock_fd);
}

/* garbage collection */

void ChatClient::cleanup()
{
    if (client_handle) delete client_handle;
    if (server_handle) delete server_handle;
    if (buffer_in)     delete buffer_in;
    if (buffer_out)    delete buffer_out;
}

/* destructor */

ChatClient::~ChatClient()
{
    cleanup();
}
