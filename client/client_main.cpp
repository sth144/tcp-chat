/**************************************************************************
 *  Title: Chat Client Engine
 *  Author: Sean Hinds
 *  Date: 25 Oct, 2018
 *  Description: Engine which runs a TCP chat client using the ChatClient
 *  			class
 * ************************************************************************/

#include <csignal>

#include "chat_client.hpp"

#define MAX_MESSAGE_LENGTH 500

using namespace std;

ChatClient* client;

void sigint_handler(int);

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        cerr << "Usage: $ " << argv[0] << " hostname port" << endl;
        exit(1);
    }

    client = new ChatClient(argv[1], argv[2], MAX_MESSAGE_LENGTH);
 
    signal(SIGINT, sigint_handler);
   
    client->startup();
    
    delete client;

    return 0;
}

/* handle SIGINT by calling client object signal handler */

void sigint_handler(int signal_number)
{
    if (client) client->sigint_intercept();
}
