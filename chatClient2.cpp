/* ChatClient Program
 *
 * CSCI 4273 Fall 2014
 *
 * Programming Assignment 1: Chat Program
 *
 * Author: Christopher Jordan
 *
 * Updated: 09/26/2014
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

using namespace std;

#ifndef INADDR_NONE
#define INADDR_NONE     0xffffffff
#endif  /* INADDR_NONE */

extern int  errno;

#define BUFSIZE     128

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
    int tcp_sock = 0;                      /* active tcp socket tracker */
    char rebuf[BUFSIZE];               /* buffer for reply communications */
    string command, params;

    while (true) {
        // Read from standard input
        fgets(buf, sizeof(buf), stdin);

        // Zero out command string
        command = "";
        params = "";

        // Separate primary command from the parameters
        for(i = 0; i < BUFSIZE; i++){
            if ( buf[i] != ' ' && buf[i] != '\0' && buf[i] != '\n' ){
                // Add to string
                command += buf[i];
            }
            else {
                break;
            }
        }
        i++;
        for (; i < BUFSIZE; i++){
            if ( buf[i] != '\n' && buf[i] != '\0' && buf[i] != '\n')
                params += buf[i];
            else
                break;
        }

        // Check for non-chat commands
        if (command == "Exit"){
            // If not connect, just exit
            if( tcp_sock = 0 ){
                // Terminate chat client
                exit (0);
            }
            else{
                buf[0] = 'L';
                buf[1] = 'e';
                buf[2] = 'a';
                buf[3] = 'v';
                buf[4] = 'e';
                buf[5] = ' ';

                // Send getnext command
                write(tcp_sock, buf, sizeof(buf));

                // Clear buffer
                memset(&buf, 0, sizeof(buf));

                // Close TCP socket
                close(tcp_sock);

                // End client
                exit(0);
            }
        }
        else if (command == "Start"){
            reply = udpCom(buf, host, portnum);
            if (reply == -1){
                printf("Error Staring Session %s, session already exists\n", params.c_str());
            }
            else{
                // Add session code here
                tcp_sock = reply;
                printf("A new chat session %s has been created and you have joined this session\n", params.c_str());

            }
        }
        else if (command == "Join"){
            //Change Join command to Find
            buf[0] = 'F';
            buf[1] = 'i';
            buf[2] = 'n';
            buf[3] = 'd';
            reply = udpCom(buf, host, portnum);
            if (reply == -1){
                printf("Error Joining Session %s, session not found \n" , params.c_str());
            }
            else{
                // Add session code here
                tcp_sock = reply;
                printf("You have joined the chat session " "%s\n", params.c_str());
            }
        }
        else if (command == "Submit"){
            if (tcp_sock == 0){
                printf("%s\n", "Sorry, you're not currently connected to a chat session");
            }
            else{
                //For Testing
                //printf("I equals: " "%i\n", i);

                // Convert submit length to string
                int sub_l_n = i - 7;
                stringstream ss;
                ss << sub_l_n;
                string sub_l_s = ss.str();

                // For testing
                //printf("Parameters: ""%s\n", params.c_str());

                // Create command string and place in buffer
                string submit = command + " " + sub_l_s + " " + params + "\0";

                //printf("Sumbit string: " "%s\n", submit.c_str());

                // For Testing
                //printf("%s\n", submit.c_str());

                // Send submit command
                write(tcp_sock, submit.c_str(), BUFSIZE);

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

                string reply_str = string(rebuf);

                if (reply_str == "Sorry, there are no new messages to retrieve" ){
                     // Display to user
                    reply_str += '\n';
                    
                    // Print message to user
                    printf("%s", reply_str.c_str());
                    
                    // Clear buffer
                    memset(&rebuf, 0, sizeof(rebuf));
                }
                else{
                    // Read reply
                    while (1){
                        // Display to user
                        printf("%s", rebuf);

                        // Clear buffer
                        memset(&rebuf, 0, sizeof(rebuf));

                        // Get next reply
                        read(tcp_sock, &rebuf, sizeof(rebuf));

                        // Convert to string again
                        reply_str = string(rebuf);

                        if( rebuf[0] == '8' && rebuf[1] == 'a' && rebuf[2] == '5' && rebuf[3] == 'Z' ){
                            // Clear buffer
                            memset(&rebuf, 0, sizeof(rebuf));
                            break;
                        }
                    }
                }
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
        printf("%s\n", "Waiting for Chat Coordinator to reply");

        // Recieve reply from server
        recvlen = recvfrom(udp_sock, rebuf, BUFSIZE, 0, (struct sockaddr *)&cli_sin, &rec_len);
        
        // Continue if the message isn't empty
        if (recvlen > 0) {
            rebuf[recvlen] = 0;
            
            //For server testing, can remove of leave later
            //printf("received message: " "%s\"\n", rebuf);

            // If first char of reply is '-' error getting port
            if ( rebuf[0] == '-'){
                // Clear buffer
                memset(&rebuf, 0, sizeof(rebuf));

                return -1;
            }

            // Connect TCP session
            tcp_sock = connectSession(host, rebuf);
            
            // For testing
            //printf("Connected session on TCP Socket: " "%i\"\n", tcp_sock);

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