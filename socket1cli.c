// firewall-cmd --permanent --add-port=3490/tcp
// systemctl restart firewalld

// The same port needs to be open on server and client machine for 2 way communications

// http://beej.us/guide/bgnet/html/single/bgnet.html#simpleserver
// gcc -o socket1cli.exe socket1cli.c
// ./socket1cli.exe


/*
	Name:		
	Purpose:		
	Arguments:	
	Called By: 	
	Calls To: 	
				
	Notes:		
				
				
	Returns: 		
	Pseudocode:	
				
				
				
				
				
				
				
				
				
				
				
				
				
				
				
				

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

/////////////////////////// Declares //////////////////////////////////////////////////////
#define SVRADDR "192.168.1.2"
#define PORT "3490" // the port client will be connecting to 
#define MAXDATASIZE 100 // max number of bytes we can get at once



/////////////////////////// Globals ///////////////////////////////////////////////////////
char buffer[MAXDATASIZE] ;

/////////////////////////// Prototypes ////////////////////////////////////////////////////


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
/*
	Name:		get_in_addr
	Purpose:	Get client return address (Where to send reply)
	Arguments:	sockaddr
	Called By: 	main
	Calls To: 	None
	Notes:		Original code taken from http://beej.us/guide/bgnet/html/single/bgnet.html#simpleserver
				
				
	Returns: 	Address structure 
	Pseudocode:	If passed a IP4 address
		return an IP4 address
	return an IP6 address
*/


	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr) ;
	}
	
	return &(((struct sockaddr_in6*)sa)->sin6_addr) ;
}

int main()
{
/*
	Name:		main
	Purpose:	Connect to a server socket and receive a string from the svr
	Arguments:	None
	Called By: 	None
	Calls To: 	get_in_addr
	Notes:		Original code taken from http://beej.us/guide/bgnet/html/single/bgnet.html#simpleserver
				hints: http://man7.org/linux/man-pages/man3/getaddrinfo.3.html
				The addrinfo structure used by getaddrinfo() contains the following fields:
				struct addrinfo
				{
					int              ai_flags;
					int              ai_family;
					int              ai_socktype;
					int              ai_protocol;
					socklen_t        ai_addrlen;
					struct sockaddr *ai_addr;
					char            *ai_canonname;
					struct addrinfo *ai_next;
				}
				struct addrinfo is returned by getaddrinfo(), and contains, on success, a linked list of such structs for a specified hostname and/or service. 
				struct addrinfo is a linked list of listeners at the given ip address and port
				inet_ntop - convert IPv4 and IPv6 addresses from binary to text form
				
The client needs to open a connection
Read a string from the connection
close the connection
While the string > 25
	If the string < 25
		exit the thread (EOF)
	else
		Parse the string and build a SQL insert
		The client needs to open a connection
		Wrtie a SQL insert to the server
		read another string from the server
				
				
				
				
	Returns: 	int
	Pseudocode:	Zero the hints struct
				fill in the hints struct
				If the call to getaddrinfo fails to get the IP4 address of the server
					Print an error
					Exit code
				
				
				
				
				
				
				
				
				
				
				
				
				
				
				
				
				
				
				
				
				
				
				
*/

int sockfd = 0 ;
int numbytes = 0 ; 
int rv = 0 ;
int i = 0 ;
int j = 0 ;
int k = 0 ;
char buff[MAXDATASIZE/2] ;
char s[INET6_ADDRSTRLEN] ;
struct addrinfo hints ;
struct addrinfo *servinfo ;
struct addrinfo *p ;

	buffer[0] = 120 ;	// x = 120
	buffer[1] = '\0' ;
	
	memset(&hints, 0, sizeof hints) ;	// Zero the hints struct
	hints.ai_family = AF_UNSPEC ;		// fill in the hints struct
	hints.ai_socktype = SOCK_STREAM ;
	
	if ((rv = getaddrinfo(SVRADDR, PORT, &hints, &servinfo)) != 0) 	// If the call to getaddrinfo fails to get the IP4 address of the server
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)) ;		// Print an error
		return 1 ;														// Exit code
	}
	
	for (i=0; i<10; i++)
	{
		for(p=servinfo; p!=NULL; p=p->ai_next)		// loop through all the results and connect to the first we can
		{
			if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			{
				perror("client: socket error") ;
				continue ;
			}
			
			if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
			{
				close(sockfd) ;
				perror("client: connect error") ;
				continue ;
			}
			
			break ;
		}
		
		if (p == NULL)
		{
			fprintf(stderr, "client: failed to connect\n") ;
			return 2 ;
		}
		
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s) ;	// The client needs to open a connection
		
		//printf("client: connecting to %s\n", s) ;
		
		printf("Buffer = %s\n", buffer) ;
		
		// Send msg back to svr
		if (send(sockfd, buffer, MAXDATASIZE, 0) != -1)		// If NOT an error sending msg to server
				printf("Sent %s to server\n", buffer) ;								//		Print send msg
		
		memset(buff, 0, MAXDATASIZE/2) ;
		memset(buffer, 0, MAXDATASIZE) ;
		
		if ((numbytes = recv(sockfd, buff, (MAXDATASIZE/2)-1, 0)) == -1)	// Read a string from the connection
		{
			perror("Receive Error, likely server finished file\n") ;
			exit(1) ;
		}
		else
			printf("Client received %s\n", buff) ;
		
		// Parse received string
		strcpy(buffer, buff) ;
		
		k = strlen(buff) ;
		printf("Strlen = %d\n", k) ;
		rv = k ;
		buffer[k++] = 32 ; 	// space
		
		for (j=k-2; j>-1; j--)
			buffer[k++] = buff[j] ;
		
		buffer[(rv*2)+1] = '\0' ;
		
		printf("Buffer now = %s\n", buffer) ;
		
		close(sockfd) ;
	}
	
	freeaddrinfo(servinfo) ;	// all done with this structure

	return 0 ;
}

















