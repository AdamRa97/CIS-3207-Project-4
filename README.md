# Project-4-s23
## Project 4: Introduction to Network and Socket Communication  
## Project Overview  
This project extends Project 3 by introducing communication between producers and consumers across a network. In project 3, the producers and consumers were on the same computer system. In this project the use of the pipe for communication is removed and communication through an internet socket is used. The purpose of the assignment is to gain some exposure to writing programs that communicate across networks.

You'll learn a bit about network sockets in lecture and lab. Much more detailed information is available in Chapter 11 of Bryant and O'Hallaron, and Chapters 57-62 in Kerrisk (see Canvas Files: Additional Textbook References). [Beej's Guide](https://beej.us/guide/bgnet/) and [BinaryTides' Socket Programming Tutorial](https://www.binarytides.com/socket-programming-c-linux-tutorial/) are potentially useful online resources.

For now, the high-level view of network sockets is that they are communication channels between pairs of processes, somewhat like pipes. They differ from pipes in that a pair of processes communicating via a socket may reside on different machines, and that the channel of communication is bi-directional.

Much of what is considered to be "socket programming" involves the mechanics of setting up the channel (i.e., the socket) for communication. Once this is done, we're left with a socket descriptor, which we use in much the same manner as we've used descriptors to represent files or pipes. That is, we can read and write data via the socket descriptor.

In this project you will use the producer programs described and implemented in Project 3 to send their data to the consumer via the network. The consumer process from Project 3 is to be modified to include two new threads that will perform the communication with the producers and enter the data into the buffers to be “processed” by the consumer threads.

## Consumer Program Operation  
Your consumer program should take in from the command line a port number on which to listen for incoming connections. If no port number is provided, your program should listen on DEFAULT_PORT (defined in your program).

The consumer program will create all threads and buffers to be used in the program. In addition to the threads and buffers described in Project 3, two additional threads are to be created. Each of these threads will be given a connection socket descriptor enabling communication with one of the producer processes. Thus, the actual data communication from a producer to the consumer will be through these threads.



## Dispatcher and Communication Threads  
The consumer dispatcher thread will have two primary functions: 1) create the ‘listen socket descriptor’ and accept remote connections, and 2) distribute connection requests to consumer threads:  

*&nbsp;&nbsp;&nbsp;&nbsp;for 2 producer connection requests {  
    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;connected_socket = accept(listening_socket);  
    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;create communication thread passing it the connected socket as a parameter to its start routine  
&nbsp;&nbsp;&nbsp;&nbsp;}*  

The communication threads (one associated with each producer) will use their connection socket to read data from their producer and deposit the data in their buffer for ‘consumption’.   

Once a communication thread has the connection socket descriptor it will work as follows:  

*&nbsp;&nbsp;&nbsp;&nbsp;while (there's a message to read) {  
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;read message from the socket  
        &nbsp;&nbsp;&nbsp;&nbsp;if (the message is NOT the message terminator) {  
            &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;save the message data in the circular buffer;  
        &nbsp;&nbsp;&nbsp;&nbsp;} else {  
            &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;set a completion flag used by the consumer process to shut down the process.  
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;}  
&nbsp;&nbsp;&nbsp;&nbsp;}*  


## Design and Setup for Socket Communication  
It is suggested that you begin this project by implementing a single producer process and a single consumer process. These two processes should be based on the sample echo client server program code in Bryant & O’Hallaron chapter 11. This sample code will help you understand the socket functions and communication.  

Once you have this code working, you should be able to import the socket/communications code to your producer and consumer processes.  

At the beginning, as you are developing your server, you'll probably run the producer client and a consumer server program on your own computer. When doing this, your server's network address will be the loopback address of 127.0.0.1. (do some research on this).  

Once you're ready to deploy your program on a real network, please restrict yourself to the nodes on cis-linux2. Start an instance of your consumer on one of the cis-linux2 systems and run your producer instances on other systems.  

There will be a weekly deliverable to be submitted to Canvas as well.  
**WEEK 1**  
Create a test producer/consumer process set using the socket code discussed in Bryant & O’Hallaron and in class.  
Create producer code for message generation and message sending through the socket.  
Create the dispatcher code to implement the listening socket and the accept connection code.  


**Week 2: project submission**  
Create the communication threads in the consumer to read data from the producers via sockets.  
Have the communication threads update the storage buffers in the consumer.  
Test the processes on your local machine.  
Test the processes across the network with the cis_linux2 systems.  
Final program delivery: sample data runs with output files.  

