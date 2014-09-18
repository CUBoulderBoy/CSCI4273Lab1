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

#define	QLEN		  32	/* maximum connection queue length	*/
#define	BUFSIZE		4096

extern int	errno;
int		errexit(const char *format, ...);
int		udpSock(const char *portnum, int qlen);
int     tcpSock(const char *portnum, int qlen, string name, map<string, int> &servers);
int     clientMsg(char buf[BUFSIZE]);

/*------------------------------------------------------------------------
 * Main - Wait on UDP port for user to make request
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[]) {
	char *portnum = "5004";               // Standard server port number
	struct sockaddr_in fsin;              // From address of a client
	int	udp_sock;                         // Coordinator UDP Socket              
    socklen_t recvlen;                    // From-address length
    char buf[BUFSIZE];           // Input buffer
    map<string, int> servers;             // Map of server strings
	
	switch (argc) {
	case	1:
		break;
	case	2:
		portnum = argv[1];
		break;
	default:
		errexit("usage: TCPmechod [port]\n");
	}

	udp_sock = udpSock(portnum, QLEN);

    while(1){
        recvlen = recvfrom(udp_sock, buf, BUFSIZE, 0, (struct sockaddr *)&fsin, &recvlen);
        
        //For confirming proper number of bytes in testing
        //printf("received %d bytes\n", recvlen);
        
        //Continue if the message isn't empty
        if (recvlen > 0) {
                buf[recvlen] = 0;
                
                //For server testing, can remove of leave later
                //printf("received message: \"%s\"\n", buf);
                
                //Pass buffer to message handler
                clientMsg(buf);

                //Clear the buffer for next use
                memset(&buf[0], 0, sizeof(buf));
        }
    }

    return 0;
}
/*------------------------------------------------------------------------
 * udpSock - allocate & bind a server socket using UDP
 *------------------------------------------------------------------------
 */
int udpSock(const char *portnum, int qlen) {
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

    /* Map port number (char string) to port number (int) */
    if ((sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
            errexit("can't get \"%s\" port number\n", portnum);

    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        errexit("can't create socket: %s\n", strerror(errno));

    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        fprintf(stderr, "can't bind to %s port: %s; Trying other port\n",
            portnum, strerror(errno));
        sin.sin_port=htons(0); /* request a port number to be allocated
                               by bind */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
            errexit("can't bind: %s\n", strerror(errno));
        else {
            socklen_t socklen = sizeof(sin);

            if (getsockname(s, (struct sockaddr *)&sin, &socklen) < 0)
                    errexit("getsockname: %s\n", strerror(errno));
            printf("New server port number is %d\n", ntohs(sin.sin_port));
        }
    }

    return s;
}

/*------------------------------------------------------------------------
 * tcpSock - allocate & bind a server socket using TCP
 *------------------------------------------------------------------------
 */
int tcpSock(const char *portnum, int qlen, string name, map<string, int> &servers) {
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
    int cport_num;

    /* Map port number (char string) to port number (int) */
    if ((sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
            errexit("can't get \"%s\" port number\n", portnum);

    /* Allocate a socket */
    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0)
        errexit("can't create socket: %s\n", strerror(errno));

    /* Bind the socket */
    // Request a port number to be allocated by bind
    sin.sin_port=htons(0);
    
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit("can't bind: %s\n", strerror(errno));
    else {
        socklen_t socklen = sizeof(sin);

        if (getsockname(s, (struct sockaddr *)&sin, &socklen) < 0)
                errexit("getsockname: %s\n", strerror(errno));
        
        cport_num = ntohs(sin.sin_port);
        printf("New server port number is %d\n", cport_num);
        servers[name] = cport_num;
    }

    return s;
}

/*------------------------------------------------------------------------
 * clientMsg - Handle client messages
 *------------------------------------------------------------------------
 */
 int clientMsg(char buf[BUFSIZE]){
    string message(buf);

    cout << message;

    return 0;
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