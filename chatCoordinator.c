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

#define	QLEN		  32	/* maximum connection queue length	*/
#define	BUFSIZE		4096

extern int	errno;
int		errexit(const char *format, ...);
int		passivesock(const char *portnum, int qlen);

/*------------------------------------------------------------------------
 * Main - Wait on UDP port for user to make request
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[]) {
	char *portnum = "5004";               // Standard server port number
	struct sockaddr_in fsin;              // the from address of a client
	int	msock;                            // master server socket
	fd_set rfds;			              // read file descriptor set
	fd_set afds;                          // active file descriptor set
	unsigned int alen;                    // from-address length
	int	fd, nfds;
    int recvlen;                          // Length of instructions recieved
    unsigned char buf[BUFSIZE];           // Input buffer
	
	switch (argc) {
	case	1:
		break;
	case	2:
		portnum = argv[1];
		break;
	default:
		errexit("usage: TCPmechod [port]\n");
	}

	msock = passivesock(portnum, QLEN);

	//nfds = getdtablesize();
	//FD_ZERO(&afds);
	//FD_SET(msock, &afds);

    while(1){
	   recvlen = recvfrom(msock, buf, BUFSIZE, 0, (struct sockaddr *)&fsin, &alen);
    }

    return 0;
}
/*------------------------------------------------------------------------
 * Passivesock - allocate & bind a server socket using UDP
 *------------------------------------------------------------------------
 */
int passivesock(const char *portnum, int qlen) {
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
 * errexit - print an error message and exit
 *------------------------------------------------------------------------
 */
int
errexit(const char *format, ...)
{
        va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}