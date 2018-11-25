#!/usr/bin/env/python

#=========================================================================================================#
#    Title: Chat Server Class Definition
#    Author:
#    Date:
#    Description: This multithreaded TCP server implements a chat service which may connect 
#                   to multiple clients at once to form a chatroom.
#    Sources Referenced: 
#    1. https://www.g-loaded.eu/2016/11/24/how-to-terminate-running-python-threads-using-signals/
#    2. https://docs.python.org/3.7/library/socket.html
#    3. https://docs.python.org/3.7/library/threading.html
#=========================================================================================================#

import socket
import sys
import select
import signal
import threading

class ChatServer():
    
    #===========================# Initialization and startup #==============================#
    
    # Constructor
    def __init__(self, host, port, server_handle, msg_length, max_threads):
        self.kill_received = False             # kill_received flag inspired by source 1 (see header)
        self.host = host
        self.port = port
        self.server_handle = server_handle
        self.msg_length = msg_length
        self.max_threads = max_threads          # only this many receiver threads allowed at a given time
        self.thread_count = 0
        self.connection_sockets = list()
        self.receiver_threads = list()
        self.dead_threads = list()              # maintain a list of dead threads to be collected
        self.event = False                      # tracks whether an event has occurred since last user prompt

        # Setup welcome socket in main thread
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))

    # Startup
    def startServer(self):
        self.server_socket.listen(10)                       # Begin listening on welcome socket in main thread
        self.alertServerListening()
        self.spawnBroadcastThread()                         # Spawn the broadcast thread
        while not self.kill_received:                       # kill_received flag inspired by source 1 (see header)
            try:
                self.spawnReceiverThread()                  # Spawn a receiver thread. Blocks until a client connects
            except KeyboardInterrupt:                       # SIGINT handling
                sys.stdout.write("\n\nSIGINT received\n\n")
                self.kill_received = True                   # kill_received flag inspired by source 1 (see header)
                self.stop()

    #=====================================# Prompts #=====================================================#

    # Notify server admin that the server is listening on specified port
    def alertServerListening(self):
        sys.stdout.write(self.server_handle + " listening on " + str(self.host) + " port " + str(self.port) + "...\n")

    # Prompt the server admin for message input to send to clients
    def promptServer(self):
        sys.stdout.write(self.server_handle + "> ")
        sys.stdout.flush()

    #=============================== Threadpool maintenance ========================-=====================#

    # Spawn the broadcast thread to broadcast messages to all clients
    def spawnBroadcastThread(self):
        self.broadcast_thread = threading.Thread(target = self.broadcastThreadFn)
        self.broadcast_thread.start()

    # Spawn a receiver thread to receive messages from a single client connection
    def spawnReceiverThread(self):
        connection_socket, addr = self.server_socket.accept()
        self.connection_sockets.append(connection_socket)               # maintain receiver sockets list
        connection_socket.settimeout(100)
        receiver_thread = threading.Thread(target = self.recevierThreadFn, args=(connection_socket,))
        self.receiver_threads.append(receiver_thread)                   # maintain receiver threads list
        if (self.thread_count < self.max_threads):      
            self.thread_count += 1
            if (len(self.dead_threads) > 3):
                self.pruneDeadThreads()                                 # run garbage collection for inactive threads
            receiver_thread.start()                                     # start the new receiver thread
        else:
            print("Maximum number of threads is " + self.max_threads)   # enforce thread limit

    # Called periodically to prune connection threads in which connection has been closed
    def pruneDeadThreads(self):
        for dead_thread in self.dead_threads:
            sys.stdout.write("joining dead thread...\n")
            dead_thread.join(1)
            self.dead_threads.remove(dead_thread)

    #=================================# Broadcast thread (single thread) #====================================#

    # Entry point for broadcast thread, which handles sending messages from server to clients
    def broadcastThreadFn(self):
        connected, self.event = True, True
        while not self.kill_received:                       # kill_received flag inspired by source 1 (see header)
            connected, self.event = self.broadcastChatFn(connected, self.event)

    # Main broadcast chat method, called in a loop within broadcastThreadFn
    def broadcastChatFn(self, connected, event):
        event = False
        sockets = [sys.stdin]       # check sys.stdin file descriptor
        # use select to asynchronously probe socket file descriptors for those that have input to read
        try:
            readable_sockets, writeable_sockets, e = select.select(sockets, [], [], 0.01)   # check for input (async)
            if (sys.stdin in readable_sockets and len(self.connection_sockets) > 0):
                (connected, event) = self.handleOutgoingMsg(self.connection_sockets)        # there is a message to send
                if (connected):
        	    self.promptServer()
        except:
            sys.stdout.write("\nServer error. Please restart the server\n")             # bad file descriptor
        return (connected, event)        

    # Handle outbound message, broadcast to clients
    def handleOutgoingMsg(self, connection_sockets):
        msg_to_send = self.server_handle + "> " + raw_input()           # format the outbound message
        disconnect = False
        if (msg_to_send == self.server_handle + "> \\quit"):        # check for disconnect
            self.serverDisconnect()                                 # handle disconnect case
        for connection_socket in connection_sockets:                # broadcast to each client
            self.sendMsg(connection_socket, msg_to_send)
        return (not disconnect, True)                  # signal to caller: connection status and whether event occurred

    # send message to client
    def sendMsg(self, connection_socket, msg_to_send):
        connection_socket.send(msg_to_send.encode())

    #========================================# Receiver threads (up to max_threads) #===================================#

    # Entry point for receiver threads, which handle incoming messages
    def recevierThreadFn(self, connection_socket):
        client_handle = self.receiveClientHandle(connection_socket)         # get client handle for this socket
        self.sendServerHandle(connection_socket, client_handle)             # respond with server handle
        self.promptServer() 
        connected = True
        while connected and not self.kill_received:         # kill_received flag inspired by source 1 (see header)
            connected, self.event = self.receiverChatFn(connection_socket, client_handle, connected, self.event)

    # Main method for receiver threads, called in a while loop within receiverThreadFn
    def receiverChatFn(self, connection_socket, client_handle, connected, event):
	try:
            if (connection_socket.fileno() > 0):                                # only send if socket is open
                event = False
                sockets = [sys.stdin, connection_socket]                        # socket file descriptors to check
                readable_sockets, w, e = select.select(sockets, [], [], 0.01)   # check sockets for input (async)
                if (connection_socket in readable_sockets):                     # there is a new inbound message
                    (connected, event) = self.handleIncomingMsg(readable_sockets, connection_socket, client_handle)
                    if (connected):
                        self.promptServer()
        except:
            pass
        return (connected, event)

    # Handle receiving of handle from client upon connection
    def receiveClientHandle(self, connection_socket):
        client_handle = connection_socket.recv(self.msg_length).decode()    # receive and decode byte message
        connection_socket.send("HANDLE_RECIEVED".encode())                  # acknowledge
        return client_handle

    # Handle sending of server handle to client during connection setup
    def sendServerHandle(self, connection_socket, client_handle):
        received_msg = self.receiveClientMsg(connection_socket)
        if (received_msg == "SEND_HANDLE"):
            successful_connect_msg = "\rClient " + client_handle + " connected"
            sys.stdout.write(successful_connect_msg + "\n")
            msg_to_send = self.server_handle
            self.sendMsg(connection_socket, msg_to_send)
            self.forwardClientMsg(connection_socket, successful_connect_msg)

    # Process message from a client
    def handleIncomingMsg(self, readable_sockets, connection_socket, client_handle):
        if (sys.stdin not in readable_sockets):     # no message from server since last event 
            sys.stdout.write("\r")                  # overwrite server prompt in stdout
        received_msg = self.receiveClientMsg(connection_socket)             # receive the message
        if (received_msg == client_handle + "> \\quit"):                    # client disconnect case
            self.handleClientDisconnect(connection_socket, client_handle)
            return (False, False)
        else:                                                               # normal message
            self.displayClientMsg(received_msg)
            self.forwardClientMsg(connection_socket, received_msg)
            return (True, True)

    # Receive a message from client during chat session
    def receiveClientMsg(self, connection_socket):
        return connection_socket.recv(self.msg_length).decode()

    # Display a message received from client
    def displayClientMsg(self, message):
        extra = len(self.server_handle) + 2 - len(message)
        if (extra < 0):
            extra = 0
        sys.stdout.write(message)
        for i in range(0, extra):                   # be sure to overwrite server prompt
            sys.stdout.write(" ")
        sys.stdout.write("\n")
        
    # Forward received message to all other connections (other threads)
    def forwardClientMsg(self, source_socket, message):
        for socket in self.connection_sockets:
            if (socket != source_socket):           # don't return to sender
                self.sendMsg(socket, message)

    #=============================# Disconnect handling and shutdown #===========================#

    # Handle the case where the server disconnects due to the admin entering '\quit' as a message
    def serverDisconnect(self):
        sys.stdout.write("\n" + self.server_handle + " disconnecting from all clients (will remain on)\n\n")
        kill_message = self.server_handle + "> \\quit"
        for connection in self.connection_sockets:
            sys.stdout.write("disconnecting connection socket...\n")
            self.sendMsg(connection, kill_message)                      # send notification to clients
            connection.close()                                          # close connections
            self.connection_sockets.remove(connection)
        sys.stdout.write("\n")
        for receiver_thread in self.receiver_threads:                   # collect receiver threads
            if (receiver_thread):
                sys.stdout.write("joining connection thread...\n")
	        receiver_thread.join(1)
                self.thread_count -= 1
        sys.stdout.write("\n")
	self.connection_sockets, self.receiver_threads = [], []
        self.alertServerListening()                                     # return to listening state

    # Handle the case where a single client disconnects
    def handleClientDisconnect(self, connection_socket, client_handle):
        sys.stdout.write("\n" + client_handle + " terminated connection\n\n")
        connection_socket.close()
        if connection_socket in self.connection_sockets:
            self.connection_sockets.remove(connection_socket)
        if (threading.current_thread() in self.receiver_threads):       # remove receiver thread from connection
            self.receiver_threads.remove(threading.current_thread())    #     threads and mark for garbage 
            self.thread_count -= 1                                      #     collection
            self.dead_threads.append(threading.current_thread())
        self.alertServerListening()

    # Shut down the server. Called by SIGINT handler
    def stop(self):
        sys.stdout.write("closing welcome socket...\n")
        self.server_socket.close()
        for connection_socket in self.connection_sockets:               # close sockets
            sys.stdout.write("closing connection socket...\n\n")
            connection_socket.close()
        sys.stdout.write("joining broadcast thread...\n")
        self.broadcast_thread.join(1)
        for thread in self.receiver_threads:                            # garbage collect receiver threads
            sys.stdout.write("joining connection thread...\n")
            thread.join(1)
            self.thread_count -= 1
        for thread in self.dead_threads:                                # garbage collect dead threads
            sys.stdout.write("joining dead thread...\n")
            thread.join(1)
        sys.stdout.write("all threads joined, main thread exiting\n\n")
        sys.stdout.write(self.server_handle + " shutting down\n")
        sys.exit(0)                                                     # shut down
