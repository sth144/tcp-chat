#!/usr/bin/python

#==============================================================================#
#    Title: Chat Server Engine
#    Author: Sean Hinds
#    Date: 25 Oct, 2018
#    Description: Starts an instance of the ChatServer class, which listens
#                    for client connections on a specified port number.
#==============================================================================#

import time

from sys import argv
from chat_server import ChatServer

# command line input validation
if (len(argv) < 2):
    print("Usage: $ " + argv[0] + " port")
    exit(1)

host = '127.0.0.1'
port = int(argv[1])
handle = 'Server'
length = 500
threads = 5

# instantiate and start the server
server = ChatServer(host, port, handle, length, threads)
server.startServer()
