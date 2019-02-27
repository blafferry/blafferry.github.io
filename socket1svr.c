// This server will serve multiple clients

// firewall-cmd --permanent --add-port=3490/tcp
// systemctl restart firewalld


// gcc -o socket1svr.exe socket1svr.c -pthread
// ./socket1svr.exe


/*
	Name:		
	Purpose:		
	Arguments:	
	Called By: 	
	Calls To: 	
				
	Notes:		
				
				
	Returns: 		
	Pseudocode:	
				
				
	
			
				
				
				
				
				
				
				
				
The server needs to open a connection
The server needs to open a file
The server needs to read a line from the file
While !EOF
	Read a string from the connection
	If the strlen > 25
		Insert the string into a DB
	Write a line to the connection
	read another line from the file
	
				
				
				
				
				
				

*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

/////////////////////////// Declares //////////////////////////////////////////////////////
#define curfile "/home/bri/data/trufx.com/EURUSD-2016-01a.csv"
#define PORT "3490"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold
#define MAXDATASIZE 100	// Size of client SQL insert string


/////////////////////////// Globals ///////////////////////////////////////////////////////

/////////////////////////// Prototypes ////////////////////////////////////////////////////
void sigchld_handler(int) ;
void *get_in_addr(struct sockaddr *sa) ;



void sigchld_handler(int s)
{
/*
	Name:		sigchld_handler
	Purpose:	reap all dead processes
	Arguments:	sockaddr
	Called By: 	main
	Calls To: 	None
	Notes:		Original code taken from http://beej.us/guide/bgnet/html/single/bgnet.html#simpleserver
				The code that's there is responsible for reaping zombie processes that appear as the fork()ed child processes exit. 
				If you make lots of zombies and don't reap them, your system administrator will become agitated.
	Returns: 	None
	Pseudocode:	Unsure, but it works
*/
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno ;

    while(waitpid(-1, NULL, WNOHANG) > 0) ;

    errno = saved_errno ;
}


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
				
				
				
				
				
				
				
				
				
				
				
	Returns: 	None
	Pseudocode:	If passed a IP4 address
		return an IP4 address
	return an IP6 address
*/

	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr) ;
	
	return &(((struct sockaddr_in6*)sa)->sin6_addr) ;
}

int main(void)
{
/*
	Name:		main
	Purpose:	Read/print a line from a client process on another machine then serve the client another string
	Arguments:	None
	Called By: 	None
	Calls To: 	get_in_addr
				sigchld_handler
	Notes:		Original code taken from http://beej.us/guide/bgnet/html/single/bgnet.html#simpleserver
				https://linux.die.net/man/3/freeaddrinfo
				addrinfo is a structure to hold servinfo.  servinfo is a linked list of available sockets
				The first message the client sends to the server isn't data, just "x" to establish a connection and get the first string
				https://linux.die.net/man/2/send
				ssize_t send(int sockfd, const void *buf, size_t len, int flags) ;
				Each time the parent receives a request, it forks a child process to process the string
				The child process will read the request, and send a new string to the client, then die
	Returns: 	int
	Pseudocode:	Initialize random number generator with the current time
				Zero the hints structure
				Fill in the hints structure
				if getaddrinfo returns anything other than 0 (an error)
					Print the malformed address
					Exit the code
				Loop through all the results
					If socket address is invalid
						Print an invalid socket address msg
						Continue to the next address
					If an invalid socket option
						Print socket option error msg
						Exit the code
					If unable to bind the socket
						Close the socket
						Print bind error msg
				Free srvinfo structure.  All done with this structure
				If p is NULL
					Print failed to bind error
					Exit code
				If not listening for a new connection
					Print listen error
					Exit code
				Reap all dead processes
				Init sa
				Reset sa flags
				If a sigaction error
					Print sigaction error msg
					Exit code
				Print waiting msg
				Endless main accept() loop
					Accept a new connection
					If an NOT an error with the new connection
						Print accept msg
					Store address of new client connection
					Print new connection msg
					Create a child process
						Close sockfd, child doesn't need the listener, let the parent listen for the next request
						Read a string from the client
						If the client reply is more than 3 characters
							print the client reply
						If NOT an error sending msg to client
							Print send msg
							Close the new client request
							Exit code
						Kill the child
				Return 0
*/

int i = 0 ;
int j = 0 ;
int sockfd = 0 ;	// listen on sock_fd for new connection request
int new_fd = 0 ;	// The parent will return a string to the requesting client
struct addrinfo hints ;
struct addrinfo *servinfo ;
struct addrinfo *p ;
struct sockaddr_storage their_addr ;	// New client request address information
socklen_t sin_size ;
struct sigaction sa ;
char buff[MAXDATASIZE/2] ;
char buffer[MAXDATASIZE] ;
char s[INET6_ADDRSTRLEN] ;
int yes = 1 ;
int rv = 0 ;
int numbytes = 0 ;
time_t t ;

char fruits[10][15] = {"Apple","Orange","Pineapple","Banana","Strawberry","Kiwi","Passion Fruit","Avocado", "Blackberry", "Olive"} ;

	srand((unsigned)time(&t)) ;		// Initialize random number generator with the current time

	memset(&hints, 0, sizeof hints) ;		// Zero the hints structure
	hints.ai_family = AF_UNSPEC ;			// Fill in the hints structure
	hints.ai_socktype = SOCK_STREAM ;
	hints.ai_flags = AI_PASSIVE ;	// use my IP
	
	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)		// if getaddrinfo returns anything other than 0 (an error)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)) ;		// Print the malformed address
		return 1 ;														// Exit the code
	}
	
	for(p=servinfo; p!=NULL; p=p->ai_next)	// Loop through all the results
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 		// If socket address is invalid
		{
			perror("server: socket invalid") ;											//		Print an invalid socket address msg
			continue ;																	//		Continue to the next address
		}

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)		// If an invalid socket option
        {
			perror("Setsockopt Error") ;														//		Print socket option error msg
			exit(1) ;																	//		Exit the code
		}
		
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)								// If unable to bind the socket
		{
			close(sockfd) ;																//		Close the socket
			perror("server: bind error") ;												//		Print bind error msg
			continue ;
		}
		
		break ;
	}
	
	freeaddrinfo(servinfo); // Free srvinfo structure.  All done with this structure
	
	if (p == NULL)							//	If p is NULL
	{
       fprintf(stderr, "server: failed to bind\n") ;	// Print failed to bind error
		exit(1) ;										// Exit code
    }
	
	if (listen(sockfd, BACKLOG) == -1)		// If not listening for a new connection
	{
		perror("listen error") ;			//		Print listen error
        exit(1);							//		Exit code
    }

	sa.sa_handler = sigchld_handler ;		// Reap all dead processes
	sigemptyset(&sa.sa_mask) ;				// Init sa
	sa.sa_flags = SA_RESTART ;				// Reset sa flags
	if (sigaction(SIGCHLD, &sa, NULL) == -1)	// If a sigaction error
	{
		perror("sigaction error") ;					//		Print sigaction error msg
		exit(1) ;								//		Exit code
	}
	
	printf("server: waiting for connections...\n") ;	// The server needs to open a connection
	
	while (1)		// Endless loop. wait for a connection
	{
		memcpy(buff, fruits[rand()%9], 15) ;
		sin_size = sizeof their_addr ;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size) ;	// Accept a new connection
		
		if (new_fd == -1)		// If an NOT an error with the new connection
		{
			perror("Error accepting connection") ;	//	Print accept msg
			continue ;
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s) ;		// Store address of new client connection
        //printf("Connection from %s\n", s) ;		// Print new connection msg
        
		if (!fork())	// Create a child process
		{
			close(sockfd) ; // Close sockfd, child doesn't need the listener, let the parent listen for the next request
			
			numbytes = recv(new_fd, buffer, MAXDATASIZE-1, 0) ;		// Read a string from the client
			
			numbytes = strlen(buffer) ;
			
			if (numbytes > 3)	// If the client reply is more than 3 characters
				printf("Child received %s\n", buffer) ;	// print the client reply
			
			
			if (send(new_fd, buff, (MAXDATASIZE/2)-1, 0) > -1)		// If NOT an error sending msg to client
				printf("Sent %s\n", buff) ;								//		Print send msg
			
			close(new_fd) ;  // Parent to keep listening for connection on sockfd
			exit (0) ;
		}
		
		close(new_fd);  // Kill the child
    }
    
    return 0 ;
}

