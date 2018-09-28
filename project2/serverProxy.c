/*
Authors: Kienan O'Brien, James Murphy
*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>

int sock_client;
int sock_telnetd;

/*
 * Opens a connection to the telnet daemon
*/
int openTelnet() {
	//create a connection to telnet
	struct sockaddr_in sin;
	sin.sin_family = PF_INET;
	//copy ip address
	in_addr_t address;
	address = inet_addr("127.0.0.1");
	if( address == -1){
		fprintf(stderr, "error setting ip adress for telnet\n");
		return 1;
	}
	memcpy(&sin.sin_addr, &address, sizeof(address));

	//set portnum------------------------------------------------------------
	sin.sin_port = htons(23);//telnet port 23

	//connect to telnet socket ----------------------------------------------
	int connectError = connect(sock_telnetd, (struct sockaddr *) &sin, sizeof(sin));
	if(connectError < 0){
		return 1;
	}
	return 0;
}

//listens to multiple ports using select
//forwards bytes from one socket to the other.
int multipleListen(int client_socket) {
	fd_set listen;           /* bit mask for listening on socket */        
	struct timeval timeout;  /* timeout for select call */       
	int nfound;              /* number of pending requests that select() found */     
	char helper[1000];

	printf("listen() socket_client : %d\n", client_socket);
	printf("listen() socket_telnet: %d\n", sock_telnetd);

	while(1) {
		/* need to wait for a message or a timeout */        
		FD_ZERO(&listen);                   /* zero the bit map */        
		FD_SET(sock_telnetd, &listen); /* telnetd socket fdset */    
		FD_SET(client_socket, &listen); /* client socket fdset */  
	     /* set seconds + micro-seconds of timeout */        
		timeout.tv_sec = 30;        
		timeout.tv_usec = 0;

		nfound = select(FD_SETSIZE, &listen, (fd_set *)0, (fd_set *)0, &timeout);

		if (nfound == 0) {            /* handle time out here... */  
			printf("timeout\n");      
		} 
		else if (nfound < 0) {            
		/* handle error here... */  
			printf("select didnt work\n");   
			return 1;   
		}
		if(FD_ISSET(client_socket, &listen)){
			printf("client message\n");
			int getter = read(client_socket, helper, 1000);
			
			if(getter == 0){
				exit(0);
			}
			
			printf("Client bytes read: %d\n", getter);
			helper[getter] = '\0';
			int writeMsg = write(sock_telnetd, helper, getter);
			if(writeMsg < 0){
				fprintf(stderr, "unable to write to server\n");
				exit(1);
			}
		}      
		if(FD_ISSET(sock_telnetd, &listen)){
			int getter = read(sock_telnetd, helper, 1000);
			
			if(getter == 0){
				exit(0);
			}
			
			printf("Telnet bytes read: %d\n", getter);
			helper[getter] = '\0';
			int writeMsg = write(sock_client, helper, getter);
			if(writeMsg < 0){
				fprintf(stderr, "unable to write to client\n");
				exit(1);
			}
		}
	}
	return 0;

}
/*
 * Waits for client to connect, then it opens a connection to the telnet
 * daemon and begins multipleListen().
*/
int startServer(int portnum) {

	//creating two sockets addresses for both the client and the telnet daemon
	struct sockaddr_in sin_toClient;
	struct sockaddr_in sin_toTelnetd;

	sin_toTelnetd.sin_family = PF_INET;
	sin_toClient.sin_family = PF_INET;
	
	sin_toClient.sin_port = htons(portnum);
	sin_toTelnetd.sin_port = htons(23);

	sin_toClient.sin_addr.s_addr = INADDR_ANY;
	
	//copy ip address
	in_addr_t address;
	address = inet_addr("127.0.0.1");
	if( address == -1){
		fprintf(stderr, "error setting ip adress\n");
		return 1;
	}
	memcpy(&sin_toTelnetd.sin_addr, &address, sizeof(address));

	int result = bind(sock_client, (struct sockaddr *)&sin_toClient, sizeof(sin_toClient));
	if (result < 0){
		fprintf(stderr,"error binding socket for client\n");
		return 1;
	}	
	//listen to client port
	result = listen(sock_client, 10);
	if(result < 0){
		fprintf(stderr, "Error listening to socket for client.\n");
		return 1;
	}

	//accept a new connection when it is received
	struct sockaddr_in client;
	size_t size = sizeof(client);
	int accepted = accept(sock_client, (struct sockaddr *) &client, (socklen_t *) &size);
	if(accepted < 0){
		printf("Could not accept client.\n");
		return 1;
	}
	printf("connection received from client, starting telnet connection\n");

	if(openTelnet() > 0){
		printf("Could not start telnet\n");
		return 1;
	}
	sock_client = accepted;
	printf("Connected to telnet, listening to multiple sockets.\n");
	return multipleListen(accepted);

}
/*
 * Reads command line arguments and sets up sockets.
*/
int main(int argc, char* argv[]){

	if(argc != 2){
		fprintf(stderr, "not correct number of commandline args\n");
	}
	int portnum;
	char* badString;
	portnum = strtol(argv[1], &badString, 0);
	if(*badString == '\0'){
		//nothing wrong
	}
	else {
		fprintf(stderr, "bad portnum\n");
		return 1;
	}

	//create sockets for both client and telnet daemon
	sock_client = socket(PF_INET, SOCK_STREAM, 6);
	sock_telnetd = socket(PF_INET, SOCK_STREAM, 6);


	if(sock_telnetd < 0) {
		fprintf(stderr, "bad socket_telnetd\n");
		return 1;
	}
	if(sock_client < 0){
		fprintf(stderr, "bad socket_desc\n");
		return 1;
	}

	return startServer(portnum);
}