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

using namespace std;

#ifndef INADDR_NONE
#define INADDR_NONE     0xffffffff
#endif  /* INADDR_NONE */

extern int  errno;

#define BUFSIZE     4096

int command(const char *host, const char *portnum);
int errexit(const char *format, ...);
int udpCom(char buf[BUFSIZE], const char *host, const char *portnum);
int connectSession(const char *host, const char *portnum);



/*------------------------------------------------------------------------
 * main - chat client over TCP and UDP
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[]) {
    char    *host = "localhost";    /* host to use if none supplied */
    char    *portnum = "5004";      /* default server port number   */

    switch (argc) {
    case 1:
        host = "localhost";
        break;
    case 3:
        host = argv[2];
        /* FALL THROUGH */
    case 2:
        portnum = argv[1];
        break;
    default:
        fprintf(stderr, "usage: TCPecho [host [port]]\n");
        exit(1);
    }

    command(host, portnum);
    exit(0);
}

/*------------------------------------------------------------------------
 * command - send command to chat coordinator and wait for reply
 *------------------------------------------------------------------------
 */
int command(const char *host, const char *portnum)
{
    char buf[BUFSIZE];                 /* buffer sending communications */
    int reply, i;                      /* socket descriptor, read count*/
    int tcp_sock;                      /* active tcp socket tracker */
    char rebuf[BUFSIZE];               /* buffer for reply communications */

    string command, params;

    while (true) {
        // For testing
        printf("waiting for command " "%s\n", "from user");

        // Read from standard input
        fgets(buf, sizeof(buf), stdin);

        // Zero out command string
        command = "";
        params = "";

        cout << "Initial buffer string: " << buf;

        // Separate primary command from the parameters
        for(i = 0; i < BUFSIZE; i++){
            if ( buf[i] != ' '){
                // Add to string
                command += buf[i];
            }
            else {
                break;
            }
        }
        i++;
        for (; i < BUFSIZE; i++){
            if ( buf[i] != '\n')
                params += buf[i];
            else
                break;
        }

        // Check for non-chat commands
        if (command == "Exit"){
            exit(0);
        }
        else if (command == "Start"){
            reply = udpCom(buf, host, portnum);
            if (reply == -1){
                printf("Error Staring Session %s, session already exists\n", params.c_str());
            }
            else{
                // Add session code here
                printf("A new chat session %s has been created and you have joined this session\n", params.c_str());

            }
        }
        else if (command == "Join"){
            reply = udpCom(rebuf, host, portnum);
            if (reply == -1){
                printf("Error Joining Session %s, session not found \n" , params.c_str());
            }
            else{
                // Add session code here
                printf("You have joined the chat session " "%s\n", params.c_str());
            }
        }
        else if (command == "Submit"){
            if (tcp_sock == 0){
                printf("%s\n", "Sorry, you're not currently connected to a chat session");
            }
            else{
                // Send submit command
                write(tcp_sock, buf, sizeof(buf));

                // Clear buffer
                memset(&buf, 0, sizeof(buf));
            }
        }
        else if (command == "GetNext"){
            if (tcp_sock == 0){
                printf("%s\n", "Sorry, you're not currently connected to a chat session");
            }
            else{
                // Send getnext command
                write(tcp_sock, buf, sizeof(buf));

                // Clear buffer
                memset(&buf, 0, sizeof(buf));

                // Read reply
                read(tcp_sock, &rebuf, sizeof(rebuf));

                // Display to user
                printf("%s\n", rebuf);

                // Clear buffer
                memset(&rebuf, 0, sizeof(rebuf));
            }
        }
        else if (command == "GetAll"){
            if (tcp_sock == 0){
                printf("%s\n", "Sorry, you're not currently connected to a chat session");
            }
            else{
                // Send getnext command
                write(tcp_sock, buf, sizeof(buf));

                // Clear buffer
                memset(&buf, 0, sizeof(buf));

                // Read reply
                read(tcp_sock, &rebuf, sizeof(rebuf));

                // Display to user
                printf("%s\n", rebuf);

                // Clear buffer
                memset(&rebuf, 0, sizeof(rebuf));
            }
        }
        else if (command == "Leave"){
            // Send getnext command
            write(tcp_sock, buf, sizeof(buf));

            // Clear buffer
            memset(&buf, 0, sizeof(buf));

            // Disable TCP socket
            tcp_sock = 0;
        }
        else{
            printf("Command: " "%s" " is invalid, please try again" "\n", command.c_str()); 
        }

        // Clear buffers
        memset(&rebuf, 0, sizeof(rebuf));
        memset(&buf, 0, sizeof(buf));
    }
}

/*------------------------------------------------------------------------
 * errexit - print an error message and exit
 *------------------------------------------------------------------------
 */
int errexit(const char *format, ...) {
        va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}

/*------------------------------------------------------------------------
 * udpCom - Function for handling UDP communications
 *------------------------------------------------------------------------
 */
int udpCom(char buf[BUFSIZE], const char *host, const char *portnum) {
    int udp_sock, recvlen, tcp_sock;
    struct sockaddr_in serv_sin, cli_sin;
    socklen_t sen_len = sizeof(struct sockaddr);
    socklen_t rec_len;
    struct hostent *phe;
    int reply;
    char rebuf[BUFSIZE];

    // Create socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Configure address for server
    memset(&serv_sin, 0, sizeof(serv_sin));
    serv_sin.sin_family = AF_INET;

    // Configure address for client
    memset(&cli_sin, 0, sizeof(cli_sin));

    // Map port number (char string) to port number (int)
    if ((serv_sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
            errexit("can't get \"%s\" port number\n", portnum);

    // Map host name to IP address, allowing for dotted decimal
    if ( phe = gethostbyname(host) ){
            memcpy(&serv_sin.sin_addr, phe->h_addr, phe->h_length);
    }
    else if ( (serv_sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
            errexit("can't get \"%s\" host entry\n", host);

    // Sent command to chat coordinator
    sendto(udp_sock, buf, strlen(buf), 0, (struct sockaddr *)&serv_sin, sen_len);

    // Clear buffer
    memset(&buf, 0, sizeof(buf));

    while(true){
        // Notify user waiting for reply
        printf("Waiting chat coordinator " "%s\"\n", "to reply");

        // Recieve reply from server
        recvlen = recvfrom(udp_sock, rebuf, BUFSIZE, 0, (struct sockaddr *)&cli_sin, &rec_len);
        
        // Continue if the message isn't empty
        if (recvlen > 0) {
            rebuf[recvlen] = 0;
            
            //For server testing, can remove of leave later
            printf("received message: " "%s\"\n", rebuf);


            // Connect TCP session
            tcp_sock = connectSession(host, rebuf);
            
            // For testing
            printf("Connected session on TCP Socket: " "%i\"\n", tcp_sock);

            // Clear buffer
            memset(&rebuf, 0, sizeof(rebuf));
            
            return tcp_sock;
        }
        else{
            continue;
        }
    }
}

/*------------------------------------------------------------------------
 * connectSession - allocate & connect a socket using TCP 
 *------------------------------------------------------------------------
 */
int connectSession(const char *host, const char *portnum){
/*
 * Arguments:
 *      host      - name of host to which connection is desired
 *      portnum   - server port number
 */
    struct hostent  *phe;   /* pointer to host information entry    */
    struct sockaddr_in sin; /* an Internet endpoint address         */
    int     s;              /* socket descriptor                    */


    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    /* Map port number (char string) to port number (int)*/
    if ((sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
        errexit("can't get \"%s\" port number\n", portnum);

    /* Map host name to IP address, allowing for dotted decimal */
    if ( phe = gethostbyname(host) )
        memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
    else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
        errexit("can't get \"%s\" host entry\n", host);

    /* Allocate a socket */
    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0)
        errexit("can't create socket: %s\n", strerror(errno));

    /* Connect the socket */
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit("can't connect to %s.%s: %s\n", host, portnum,
        strerror(errno));
    return s;
}