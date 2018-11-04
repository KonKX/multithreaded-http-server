# multithreaded-http-server

Simple multithreaded HTTP server using sockets

The project has been implemented and tested in a Linux environment.

Two source code files, .c, a shell script, a .h and makefile header file are contained.

The basic function of web creator is to create a certain number of html files with random names that contain random text and links to other html files. The web server function is to "listen" to two specific ports and to respond to simple HTTP (GET) requests or commands that are made by users.

Here's a brief description of the files ...

webcreator.sh: The web creator initially checks if all of the arguments given are correct. If the root directory is not empty then it is empty before it starts writing anything to it. If a check fails, the appropriate message is displayed to the user and the program terminates. If there is no error, the (random) names of the html files that are to be created and stored in memory are initially created. Then, for each html file, the corresponding internal and external links are created, and the files are created by adding content to them, which consists of quotations of the input dataset given as an argument and a number of internal and external links. Throughout the program run, information is printed on what is happening at the moment.

server.c: Contains the implementation of a simple multithreaded HTTP server that uses sockets. First, a certain number of threads are created in a thread pool. The threads go into blocked state until they log in and send a user request. The server initially creates two sockets on different ports where one listens to GET requests from the user side while the other in the STATS and SHUTDOWN commands. To do this, the server, after creating the sockets, binds and listens to them, then waits until a user logs on. The system call call selected () controls the file descriptors of the existing sockets and "picks up" what is available each time. So the program can know which port the user is connected to in order to be able to answer any requests he receives. When the user sends the request, the server performs some checks to see if the request is in the proper format. If the request is of the GET type (eg GET /siteA/pageA_B.html), the server thread checks whether the file that the user is requesting exists, and if it exists, looks if it has the appropriate permissions. In the first case, if the file does not exist, a 404 Not Found response is returned
 to the user while the file does not have permissions returned 403 Forbidden. If the file exists, the server places the file descriptor in a queue and "wakes up" one of the threads in the thread pool. This thread in turn goes and reads from the beginning of the queue the specific file descriptor and goes and writes to another queue the requested file sends a signal to the server that it can start reading and returning to a blocked state. The server in turn goes and reads from the beginning of the second queue the file, adds an HTTP header and sends the response back to the user who requested the file. In the next request that comes the server gets to awaken the next thread (in the same order that was created) and when exhausted, all the threads start again from the beginning in the same order. The server continuously receives requests until the SHUTDOWN command is given through the command port to which it is listening by releasing the memory that it has committed by ending its operation. Finally, the STATS command displays information about how long the server is running, how many pages it has returned, and the total number of bytes.

client.c: A utility that is used to connect a user to the server, send an HTTP request, and receive an answer. After each request the program terminates so the user must run it whenever he wants to send a request.
It accepts the server address (eg linux01.di.uoa.gr) and the port number to which the user wants to connect.

server.h: Contains the statements of the structures and functions of the program along with the necessary libraries.

Make and make clean compilation command for deleting object files.

./myhttpd -p <serving_port> -c <command_port> -t <num_of_threads> -d <root_dir>

./client <server_address> <port>
