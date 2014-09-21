#include <arpa/inet.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

using namespace std;

#ifndef INADDR_NONE
#define INADDR_NONE     0xffffffff
#endif  /* INADDR_NONE */

extern int	errno;

int	command(const char *host, const char *portnum);
int	errexit(const char *format, ...);
int	udpSock(const char *host, const char *portnum);

#define BUFSIZE     4096

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
	int	udp_sock, n, recvlen;			       /* socket descriptor, read count*/
	int outchars, inchars;	           /* characters sent and received	*/
    struct sockaddr_in serv_sin, cli_sin;
    socklen_t sen_len = sizeof(struct sockaddr);
    socklen_t rec_len;
    struct hostent *phe;               /* pointer to host information entry    */

    // Create socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Configure address for server
    memset(&serv_sin, 0, sizeof(serv_sin));
    serv_sin.sin_family = AF_INET;

    // Configure address for client
    memset(&cli_sin, 0, sizeof(cli_sin));
    //cli_sin.sin_family = AF_INET;

    // Map port number (char string) to port number (int)
    if ((serv_sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
            errexit("can't get \"%s\" port number\n", portnum);

    // Map host name to IP address, allowing for dotted decimal
    if ( phe = gethostbyname(host) ){
            memcpy(&serv_sin.sin_addr, phe->h_addr, phe->h_length);
            //memcpy(&cli_sin.sin_addr, phe->h_addr, phe->h_length);
    }
    else if ( (serv_sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
            errexit("can't get \"%s\" host entry\n", host);

	while (true) {
        // For testing
        printf("waiting for command " "%s\n", "from user");

        // Read from standard input
        fgets(buf, sizeof(buf), stdin);

		// Ensure line null-terminated
        buf[BUFSIZE] = '\0';

        // Sent command to chat coordinator
		sendto(udp_sock, buf, strlen(buf), 0, (struct sockaddr *)&serv_sin, sen_len);

        // Clear buffer
        memset(&buf, 0, sizeof(buf));

		while(true){
            // For testing purposes
            printf("waiting for reply " "%s\n", "from coordinator");
            
            // Recieve reply from server
            recvlen = recvfrom(udp_sock, buf, BUFSIZE, 0, (struct sockaddr *)&cli_sin, &rec_len);
            
            //Continue if the message isn't empty
            if (recvlen > 0) {
                buf[recvlen] = 0;
                
                //For server testing, can remove of leave later
                printf("received message: \"%s\"\n", buf);

                //Clear the buffer for next use
                memset(&buf, 0, sizeof(buf));

                break;
            }
            else{
                continue;
            }
        }
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
 * connectsock - allocate & connect a socket using TCP 
 *------------------------------------------------------------------------
 */
int udpSock(const char *host, const char *portnum) {
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
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
                errexit("can't create socket: %s\n", strerror(errno));

    /* Connect the socket */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
                errexit("can't connect to %s.%s: %s\n", host, portnum,
                        strerror(errno));
        return s;
}