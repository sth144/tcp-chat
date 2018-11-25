***********************************************************************************************************
			
			TCP Chat Application: Python Server and C++ Client 
		A Network Application built on the Transmission Control Protocol
					Sean T. Hinds
					25 Oct, 2018

***********************************************************************************************************

					Overview

This pair of programs use the TCP transport layer protocol to implement a chat service network application.
The server is written in Python (3.7) and the client is written in C++(11). The programs are currently 
configured to be run on the same host (127.0.0.1), although they could be adapted to run on separate end 
systems. These programs have only been tested on the flip3.engr.oregonstate.edu Linux server, but should be
portable to most *nix environments, and adaptable to Windows environments. The server is multithreaded, and
can accept up to 5 concurrent connections.

The two programs are implemented by 6 files, all within the chat_app directory:

chat_app/
	server.py
	chat_server.py
	makefile ----(builds)----> client (executable)
	client_main.cpp-------------^
	chat_client.cpp-------------^
	chat_client.hpp
	

					Instructions

Setup
The server program must be started before attempting to connect to the server through the client program. 
Both programs can be executed within the same directory in two separate terminal windows (instructions 
for each below). 

1. To begin, open two terminal windows within the chat_app directory, one to run the server,
and another to run the client. 
2. In either terminal, compile and link the client executable by running:	
	$ make
3. It may be necessary to run the following to enable running the Python server as an executable:
	$ chmod -x server.py

Server (Python)
3. In the server window, start the server with the following command:
	$ ./server.py {port}
Select any port you wish for the server to use to listen for connections. Host is localhost, and does not
need to be specified
4. You should see a message that the server is listening on the port specified from the command line if
everything has worked. If so, the server has a welcoming socket awaiting incoming connections, when a 
client connects to this exposed socket, it will be connected to a connection socket allocated by the server,
and the welcoming socket will resume listening for connections. it's time to start the client.

Client (C++)
5. In the client window, start the client with the following command:
	$ ./client {host} {port}
When launching the client, you do need to specify the host name, which will be 127.0.0.1. The port number
will be whatever port the server is currently listening on.
6. The client will prompt you to enter a handle, which must be a string of 1 to 10 characters with no 
spaces (client program will validate input). Once entered, this handle is sent to the server process over
the connection socket. This handle will be prepended to all messages from the client sent to the server and
displayed on the client. The server receives this handle and acknowledges receipt by sending its own handle,
which is hardcoded to "Server". The client and server should display messages affirming that they are now
connected, and should display an input prompt with each's respective handle

(Server)					(Client)
Client Jimmy connected				Server connected
Server>_					Jimmy>_

7. There is no required order of messages between the client and server. Either the client or server may 
send the first message, and either the client or server may send any number of consecutive messages with 
no issue. The outputs will show a log of all messages received, with a prompt for user input at the bottom.
When one end sends a message to the other, a carriage return is outputted to stdout in the receiving 
process before printing the message received, which causes the prompt to be overwritten. This means the log
will not be polluted with 'unused prompts':

(Server)					(Client)
Client Jimmy connected				Server connected
Jimmy> Me first!				Jimmy> Me first!
Jimmy> And Second!				Jimmy> And Second!
Server> My turn					Server> My turn
Jimmy> My turn again				Jimmy> My turn again
Server>_					Jimmy>_

Send a few messages from one end to the other to test. Messages may be up to 500 characters.
8a. If you wish to quit, enter the message '\quit' (no quotes) on either the client or the server, to 
disconnect from either end. Disconnecting from the server side will terminate all client connections,
and the server will remain listening on the welcoming socket (user will be notified of this). Disconnecting
from the client side will terminate the connection of whatever client entered the message, but will 
leave any other client connections unaffected. The client which disconnected will be prompted to 
reconnect if they wish. If they select 'Y', they will start over from entering a handle (step 6), if 
they select 'N', the client process will end.
8b. If you wish, you may initiate additional client connections by opening a new terminal in the chat_app
directory and beginning from step 6 above (you will likely want to enter a new handle for each client).
Up to 5 clients may be connected at once. When the server sends a message, that message is broadcast to 
every connected client. When a client sends a message, that message is forwarded on the server to every
connected client:

(Server)			(Client 1)			(Client 2)
...				...				
Client Jimmy connected		Server connected		...
Jimmy> Me first!		Jimmy> Me first!		$ ./client 127.0.0.1 4500
Jimmy> And Second!		Jimmy> And Second!		...
Server> My turn			Server> My turn			Server> My turn
Jimmy> My turn again		Jimmy> My turn again		Jimmy> My turn again
Timmy> Hey guys!		Timmy> Hey guys!		Timmy> Hey guys!
Server> I'm the server		Server> I'm the server		Server> I'm the server
Server>_			Jimmy>_				Timmy>_

* note that clients are essentially 'peers', and that this is a hybrid peer-to-peer architecture mediated
by a server

8c. At any point, you may enter Ctrl+C to stop the server. The server is configured to catch SIGINT 
signals and exit gracefully. All clients are notified to terminate so they may deallocate memory, all
socket connections are closed, all threads are joined, and the server process exits.

Extra Credit Features
The extra credit features implemented in this application are
1. Set up your programs so that either host can make first contact (?). If I'm interpreting that spec
correctly, then I believe I fulfilled it. Either host can send the first message.
2. Make it possible for either host to send at any time without "taking turns"
3. Make your server multi-threaded

					Sources

The following sources were referenced during development (all code is original):
C++ sockets: Beej's Guide to Network Programming 
	https://beej.us/guide/bgnet/html/multi/index.html 
Python sockets: Python socket Module Documentation
	https://docs.python.org/3/library/socket.html
Python threading: Python threading Module Documentation
	https://docs.python.org/3.7/library/threading.html
Python signal handling with threads: George Notora's blog post on G-Loaded Journal from 24 Nov, 2016
	https://www.g-loaded.eu/2016/11/24/how-to-terminate-running-python-threads-using-signals/