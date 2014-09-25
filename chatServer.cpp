/* ChatServer Program
 *
 * CSCI 4273 Fall 2014
 *
 * Programming Assignment 1: Chat Program
 *
 * Author: Christopher Jordan
 *
 * Updated: 09/23/2014
 */

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>

#include <netinet/in.h>
#include <netdb.h>

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <ctime>

using namespace std;

#define	QLEN		  32	/* maximum connection queue length	*/
#define	BUFSIZE		 128
#define TIMEOUT       10

extern int	errno;
int		errexit(const char *format, ...);
int     clientCom(int fd, map<int, string> &chat_msgs, map<int, int> &cli_read, int &latest_msg);
int     udpSock();

/*------------------------------------------------------------------------
 * Main - Chat session server for single chat instance
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[]) {
    struct sockaddr_in fsin;            // Address of a client
    fd_set rfds;                        // Read file descriptor set
    fd_set afds;                        // Active file descriptor set
    unsigned int alen;                  // From address length
    int fd, nfds;                       // File trackers
    int tcp_sock, ssock, udp_sock, s_udp_sock;      // Socket trackers
    int reply;                          // Reply variable
    int latest_msg = 0;                 // Tracker for the last message submitted
    map<int, string> chat_msgs;         // Map to store chat messages
    map<int, int> cli_read;             // Map to track client message reads
    unsigned long timer;                // Time to check for terminate
    string s_name;                      // Session name
    char term_buf[BUFSIZE];             // Buffer for terminate command
    string term_str;                    // Terminate command string
    char    *host = "localhost";
    struct timeval timeout = {TIMEOUT, 0};
    int time;

    // For Testing
    //printf("%s\n", "Starting server instance");
    //printf("Server UDP Socket" "%s\n", argv[2]);

    // Get tecp socket from provided arguements
    tcp_sock = atoi(argv[1]);
    s_udp_sock = atoi(argv[2]);
    s_name = string(argv[3]);

    // Create UDP socket
    udp_sock = udpSock();

    struct sockaddr_in serv_sin;
    socklen_t sen_len = sizeof(struct sockaddr);
    struct hostent *phe;

    // Create socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Configure address for server
    memset(&serv_sin, 0, sizeof(serv_sin));
    serv_sin.sin_family = AF_INET;

    // Map port number (char string) to port number (int)
    if ((serv_sin.sin_port=htons((unsigned short)s_udp_sock)) == 0)
            errexit("can't get \"%s\" port number\n", argv[2]);

    // Map host name to IP address, allowing for dotted decimal
    if ( phe = gethostbyname(host) ){
            memcpy(&serv_sin.sin_addr, phe->h_addr, phe->h_length);
    }
    else if ( (serv_sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
            errexit("can't get \"%s\" host entry\n", host);

    // Notify the the chat session has been started
    //printf("Starting chat session with socket %d\n", tcp_sock);

    // Initialize variables for network connections
    nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(tcp_sock, &afds);

    // Set clock to start
    timer = (unsigned long)clock();

    while (1) {
        // For testing
        //printf("%s\n", "Before memcopy");

        memcpy(&rfds, &afds, sizeof(rfds));

        if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, &timeout) < 0){
            errexit("select: %s\n", strerror(errno));
        }

        // For testing
        //printf("%s\n", "Before FD_ISSET");

        // Set time passed in seconds
        time = ((unsigned long)clock() - timer) / CLOCKS_PER_SEC;

        // For testing
        //printf("Time elapsed: " "%i\n", time);

        // If no communications in the last minute, terminate chat session
        if ( time >= 60 )
        {
            // Notify user that chat server session is terminating
            printf("Terminating chat session %s\n", s_name.c_str());

            // Prepare termination command
            term_str = "Terminate " + s_name;
            strncpy(term_buf, term_str.c_str(), sizeof(term_buf));

            // Send terminate command to chat coordinator
            sendto(udp_sock, term_buf, strlen(term_buf), 0, (struct sockaddr *) &serv_sin, sen_len);

            // close sockets with all clients
            for (fd = 0; fd < nfds; ++fd)
            {
                if (fd != tcp_sock && FD_ISSET(fd, &afds))
                {
                    //printf("closing tcp connection with client %d\n", fd);
                    close(fd);
                    FD_CLR(fd, &afds);
                }
            }
            close(tcp_sock);
            exit(0);
        }

        if (FD_ISSET(tcp_sock, &rfds)) {
            // Create socket variable for inbound socket
            int ssock;

            // Set size of from address
            alen = sizeof(fsin);

            // Accept new connection on the socket
            ssock = accept(tcp_sock, (struct sockaddr *)&fsin, &alen);

            // For testing
            //printf("Accepted communication on TCP socket ""%i\n", ssock);
            
            // If socket is less than zero than the socket is invalid and error
            if (ssock < 0){
                errexit("accept: %s\n", strerror(errno));
            }

            // Intialize read message index for this client
            cli_read[ssock] = 0;

            // Set socket to active file descriptor
            FD_SET(ssock, &afds);
        }

        // For testing
        //printf("%s\n", "Starting FD loop");

        for (fd = 0; fd < nfds; ++fd){
            //Verify that the fd doesn't make to the server socket
            if (fd != tcp_sock && FD_ISSET(fd, &rfds)){

                // For testing
                //printf("%s\n", "About to call clientCom function");

                // Call client communication handler function
                reply = clientCom(fd, chat_msgs, cli_read, latest_msg);

                // For testing
                //printf("%s\n", "Out of clientCom function");

                // If return is 0, terminal connection to client
                if (reply == 0) {
                    // Close connection to client
                    (void) close(fd);
                    FD_CLR(fd, &afds);

                    // Remove client from tracker
                    cli_read.erase(ssock);
                }

                // If return is 1 reset clock
                if (reply == 1){
                    timer = clock();
                }
            }
        }

        //printf("%s\n", "Ending FD loop");
    }
}

/*------------------------------------------------------------------------
 * errexit - print an error message and exit
 *------------------------------------------------------------------------
 */
int errexit(const char *format, ...){
        va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}

/*------------------------------------------------------------------------
 * clientCom - Handle recieved client communications
 *------------------------------------------------------------------------
 */
int clientCom(int fd, map<int, string> &chat_msgs, map<int, int> &cli_read, int &latest_msg){
    char buf[BUFSIZE];                  // Recieve buffer
    char rebuf[BUFSIZE];                // Send buffer
    int recvlen;                        // Length of message recieved
    int i, k, index;                    // Iterators for parsing
    string command, params, reply;      // String holders
    stringstream ss;                    // Stringstream

    // Determine the size of the message recieved
    recvlen = recv(fd, buf, sizeof(buf), 0);

    // For testing
    //printf("Buffer recieved: " "%s\n", buf);

    // If size of message received is 0, error occured
    if (recvlen < 0)
    {
        printf("Error: No message received %s\n", strerror(errno));
        return -1;
    }

    // Clear strings before usage
    command = "";
    params = "";

    //Separate primary command from the parameters
    for(i = 0; i < recvlen; i++){
        if ( buf[i] != ' ' && buf[i] != '\0' && buf[i] != '\n'){
            // Add command to string
            command += buf[i];
        }
        else{
            break;
        }
    }

    // For testing
    //printf("Command recieved: " "%s\n", command.c_str());

    // Remove whitespace
    i++;

    //Store place marker for submit command
    k = i;

    // Parse out parameters
    for (; i < recvlen; i++){
        params += buf[i];
    }

    if (command == "Submit"){
        // Parse out message length
        string msg_l_str = "";
        string msg = "";
        int msg_l_int;
        
        for (i = k; i < recvlen; i++){
            if ( buf[i] != ' ' && buf[i] != '\0' && buf[i] != '\n' ){
                msg_l_str += buf[i];
            }
            else{
                break;
            }
        }

        // Remove space before message
        i++;

        // Convert string length to int
        msg_l_int = atoi(msg_l_str.c_str());
        msg_l_int += i;

        // Get message from buffer
        for (; i < msg_l_int; i++){
            msg += buf[i];
        }

        // Map message into storage
        chat_msgs[latest_msg] = msg;
        latest_msg++;

        // For Testing
        //printf("Message stored: " "%s\n", msg.c_str());

        return 1;
    }
    else if (command == "GetNext"){
        // For testing
        //printf("%s\n", "GetNext Command recieved");

        // Get last read message for client
        index = cli_read[fd];

        // For testing
        //printf("Index retrieved: " "%i\n", index);

        // Clear buffer
        memset(&rebuf, 0, sizeof(rebuf));

        // For testing
        //printf("%s\n", chat_msgs[index].c_str());

        if (chat_msgs.count(index) == 1){
            // Create reply with message number followed by message
            reply = chat_msgs[index];

            // Increase the read index for the client
            cli_read[fd] = index + 1;

            // Copy string into reply buffer
            strncpy(rebuf, reply.c_str(), sizeof(rebuf));

            // Send reply to client
            send(fd, rebuf, strlen(rebuf), 0);

            return 1;
        }
        else{
            reply = "Sorry, there are no new messages to retrieve";

            // Copy string into reply buffer
            strncpy(rebuf, reply.c_str(), sizeof(rebuf));

            // Send reply to client
            send(fd, rebuf, strlen(rebuf), 0);

            return 1;
        }
    }
    else if (command == "GetAll"){
        // Get last read message for client
        index = cli_read[fd];

        if (chat_msgs.count(index) == 1) {
            for (; index < latest_msg; index++){
                // Clear buffer
                memset(&rebuf, 0, sizeof(rebuf));

                // Create reply
                reply = chat_msgs[index];
                reply += "\n";

                // Increase the read index for the client
                cli_read[fd] = index + 1;

                // Copy string into reply buffer
                strncpy(rebuf, reply.c_str(), sizeof(rebuf));

                // Send reply to client
                send(fd, rebuf, strlen(rebuf), 0);
            }
            // Clear buffer
            memset(&rebuf, 0, sizeof(rebuf));

            // Create end message
            reply = "END";
            reply += "\n";

            // Copy string into reply buffer
            strncpy(rebuf, reply.c_str(), sizeof(rebuf));

            // Send reply to client
            send(fd, rebuf, strlen(rebuf), 0);

            return 1;
        }

        else{
            reply = "Sorry, there are no new messages to retrieve";

            // Clear buffer
            memset(&rebuf, 0, sizeof(rebuf));

            // Copy string into reply buffer
            strncpy(rebuf, reply.c_str(), sizeof(rebuf));

            // Send reply to client
            send(fd, rebuf, strlen(rebuf), 0);

            return -1;
        }
    }
    else if (command == "Leave"){
        // For testing
        //printf("%s\n", "Recieved leave command");

        // Return terminating command
        return 0;
    }
    else{
        // For testing
        //printf("%s\n", "Command fell through");

        // Invalid command
        return -1;
    }
}

/*------------------------------------------------------------------------
 * udpSock - allocate & bind a server socket using UDP
 *------------------------------------------------------------------------
 */
int udpSock() {
/*
 * Arguments:
 *      portnum   - port number of the server
 *      qlen      - maximum server request queue length
 */
    struct sockaddr_in sin; /* an Internet endpoint address  */
    int     s;              /* socket descriptor             */

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;

    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        errexit("can't create socket: %s\n", strerror(errno));

    /* Bind the socket */
    sin.sin_port=htons(0);
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit("can't bind: %s\n", strerror(errno));
    else {
        socklen_t socklen = sizeof(sin);

        if (getsockname(s, (struct sockaddr *)&sin, &socklen) < 0)
                errexit("getsockname: %s\n", strerror(errno));
        //printf("New server port number is %d\n", ntohs(sin.sin_port));
    }

    return s;
}