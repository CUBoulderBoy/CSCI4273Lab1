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

extern int	errno;

#define BUFSIZE     4096

int	command(const char *host, const char *portnum);
int	errexit(const char *format, ...);
int udpCom(char buf[BUFSIZE], const char *host, const char *portnum);



/*------------------------------------------------------------------------
 * main - TCP client for ECHO service
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[]) {
	char	*host = "localhost";	/* host to use if none supplied	*/
	char	*portnum = "5004";	    /* default server port number	*/

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
	char buf[BUFSIZE];		           /* buffer for one line of text	*/
	int	reply, i;			                   /* socket descriptor, read count*/

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

        cout << "Command recieved: " << command;

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
            reply = udpCom(buf, host, portnum);
            if (reply == -1){
                printf("Error Joining Session %s, session not found \n" , params.c_str());
            }
            else{
                // Add session code here
                printf("You have joined the chat session " "%s\n", params.c_str());
            }
        }
        else{
            printf("Command: " "%s" " is not currently supported" "\n", command.c_str()); 
        }

		// Ensure line null-terminated
        buf[BUFSIZE] = '\0';
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
    int udp_sock, recvlen;
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

    // Notify user of attempt to connect
    printf("Sending session request to the " "%s\n", "chat coordinator");

    while(true){
        // For testing purposes
        //printf("waiting for reply " "%s\n", "from coordinator");
        
        // Recieve reply from server
        recvlen = recvfrom(udp_sock, rebuf, BUFSIZE, 0, (struct sockaddr *)&cli_sin, &rec_len);
        
        // Continue if the message isn't empty
        if (recvlen > 0) {
            rebuf[recvlen] = 0;
            
            //For server testing, can remove of leave later
            printf("received message: " "%s\"\n", rebuf);

            // Return buffer as int
            stringstream ss(rebuf);
            ss >> reply;

            // Clear buffer
            memset(&rebuf, 0, sizeof(rebuf));

            if (!reply){
                printf("Error converting string to " "%s\"\n", "number");
                return 1000;
            }
            else{
                return reply;
            }
        }
        else{
            continue;
        }
    }
}