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
#define	BUFSIZE		4096

extern int	errno;
int		errexit(const char *format, ...);
int		passivesock(const char *portnum, int qlen);
int     clientCom(int fd, map<int, string>* chat_msgs, map<int, int>* cli_read);

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
    int tcp_sock, ssock;                // Socket trackers
    int reply;                          // Reply variable
    map<int, string> chat_msgs;         // Map to store chat messages
    map<int, int> cli_read;             // Map to track client message reads
    time_t timer;                       // Time to check for terminate

    // Get tecp socket from provided arguements
    tcp_sock = atoi(argv[1]);

    // Listen on the provided tcp socket for client communications
    if (listen(tcp_sock, QLEN) < 0)
        errexit("Can't listen on socket: %s\n", strerror(errno));

    // Notify the the chat session has been started
    printf("Starting chat session with socket %d\n", tcp_sock);

    // Initialize variables for network connections
    nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(tcp_sock, &afds);

    // Set clock to start
    timer = clock();

    while (1) {
        memcpy(&rfds, &afds, sizeof(rfds));

        if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0){
            errexit("select: %s\n", strerror(errno));
        }

        if (FD_ISSET(tcp_sock, &rfds)) {
            // Create socket variable for inbound socket
            int ssock;

            // Set size of from address
            alen = sizeof(fsin);

            // Accept new connection on the socket
            ssock = accept(tcp_sock, (struct sockaddr *)&fsin,
                &alen);
            
            // If socket is less than zero than the socket is invalid and error
            if (ssock < 0){
                errexit("accept: %s\n", strerror(errno));
            }

            // Intialize read message index for this client
            cli_read[ssock] = 0;

            // Set socket to active file descriptor
            FD_SET(ssock, &afds);
        }

        for (fd=0; fd<nfds; ++fd){
            //Verify that the fd doesn't make to the server socket
            if (fd != tcp_sock && FD_ISSET(fd, &rfds)){

                // Call client communication handler function
                reply = clientCom(fd, &chat_msgs, &cli_read);

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

        // If no communications in the last minute, terminate chat session
        if ( (clock() - timer / CLOCKS_PER_SEC) >= 60 )
        {
            printf("TERMINATE\n");
        }
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
int clientCom(int fd, map<int, string>* chat_msgs, map<int, int>* cli_read){
    char buf[BUFSIZE];                  // Recieve buffer
    char rebuf[BUFSIZE];                // Send buffer
    int recvlen;                        // Length of message recieved
    int i;                              // Iterator for parsing
    string command, params;             // String holders

    // Clear recieve buffer before usage
    memset(&buf, 0, sizeof(buf));

    // Determine the size of the message recieved
    recvlen = recv(fd, buf, sizeof(buf), 0);

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
        if ( buf[i] != ' '){
            // Add command to string
            command += buf[i];
        }
        else{
            break;
        }
    }
    i++;
    for (; i < recvlen; i++){
        params += buf[i];
    }

    if (command == "Submit"){
        // deal with command
        ;
    }
    else if (command == "GetNext"){
        // deal with command
        ;
    }
    else if (command == "GetAll"){
        // deal with command
        ;
    }
    else if (command == "Leave"){
        // deal with command
        ;
    }
    else{
        // Invalid command
        ;
    }


}