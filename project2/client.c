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

int sock_tel;
int sock_server;


//listens to multiple ports using select, forwards data from one
//port to the next.
int multipleListen(int tel_socket) {
	fd_set listen;           /* bit mask for listening on socket */        
	struct timeval timeout;  /* timeout for select call */       
	int nfound;              /* number of pending requests that select() found */     
	char helper[1000];
	printf("listen() socket_server : %d\n", sock_server);
	printf("listen() socket_tel: %d\n", tel_socket);
	while(1) {
		/* need to wait for a message or a timeout */        
		FD_ZERO(&listen);                   /* zero the bit map */        
		FD_SET(tel_socket, &listen); /* telnet socket fdset */    
		FD_SET(sock_server, &listen); /* server socket fdset */  
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
		if(FD_ISSET(tel_socket, &listen)){
			printf("client message\n");
			int getter = read(tel_socket, helper, 1000);
			printf("Client bytes read: %d\n", getter);
			helper[getter] = '\0';
			int writeMsg = write(sock_server, helper, getter);
			if(writeMsg < 0){
				fprintf(stderr, "unable to write to server\n");
				exit(1);
			}
		}      
		if(FD_ISSET(sock_server, &listen)){
			int getter = read(sock_server, helper, 1000);
			printf("Telnet bytes read: %d\n", getter);
			helper[getter] = '\0';
			int writeMsg = write(tel_socket, helper, getter);
			if(writeMsg < 0){
				fprintf(stderr, "unable to write to client\n");
				exit(1);
			}
		}
	}
	return 0;

}
/*
 * Handles connecting to server
*/
int connectToServer(char* hostName, int portnum){
	struct sockaddr_in sin;
	sin.sin_family = PF_INET;
	//copy ip address
	in_addr_t address;
	address = inet_addr(hostName);
	if( address == -1){
		fprintf(stderr, "error setting ip adress\n");
		return 1;
	}
	memcpy(&sin.sin_addr, &address, sizeof(address));
	//memcpy (&sin.sin_addr, hptr->h_addr, hptr->h_length); 

	//set portnum------------------------------------------------------------
	sin.sin_port = htons(portnum);

	//connect ---------------------------------------------------------------
	int connectError = connect(sock_server, (struct sockaddr *) &sin, sizeof(sin));
	if(connectError < 0){
		fprintf(stderr, "could not connect to %s\n", hostName);
		return 1;
	}

	printf("successfully connected to server\n");
	
	return 0;
}
/*
 * Waits for a connection from telnet, once telnet connects ->
 * open a connection to the server. Then start multipleListen().
*/
int connectAll(int portnum, char *hostname, int port){
	struct sockaddr_in sin_toTel;

	sin_toTel.sin_family = PF_INET;
	sin_toTel.sin_port = htons(portnum);
	sin_toTel.sin_addr.s_addr = INADDR_ANY;

	printf("binding socket...\n");
	int result = bind(sock_tel, (struct sockaddr *)&sin_toTel, sizeof(sin_toTel));
	if(result < 0){
		fprintf(stderr, "error binding socket to Telnet with %d\n", portnum);
		exit(1);
	}

	printf("listening for a connection...\n");
	result = listen(sock_tel, 10);
	if(result < 0){
		fprintf(stderr, "error connecting to telnet\n");
		exit(1);
	}

	printf("got a connection, waiting for connection to be accepted...\n");
	struct sockaddr_in client;
	size_t size = sizeof(client);
	int accepted = accept(sock_tel, (struct sockaddr *) &client, (socklen_t *) &size);
	if(accepted < 0){
		printf("Could not accept telnet\n");
		exit(1);
	}
	printf("accepted connection from telnet\n");
	//connect to server proxy after
	if(connectToServer(hostname, port) != 0){
		return 1;
	}

	sock_tel = accepted;
	multipleListen(accepted);
	return 0;
}
/*
 * Reads command line arguments and sets up sockets.
*/
int main(int argc, char* argv[]){
	//error checking
	if(argc != 4){
		fprintf(stderr, "not correct number of commandline args\n");
		return 1;
	}
	char* hostName = "";
	hostName = argv[2];
	int portnum;
	char* badString;
	//get portnum
	portnum = strtol(argv[3], &badString, 0);
	if(*badString == '\0'){
		//printf("port num is: %d\n", portnum);
	}
	//error checking
	else{
		fprintf(stderr, "bad portnum\n");
		return 1;
	}
	int portnumTel;
	portnumTel = strtol(argv[1], &badString, 0);
	if(*badString == '\0'){
		//printf("port num is: %d\n", portnum);
	}
	//error checking
	else{
		fprintf(stderr, "bad portnum\n");
		return 1;
	}

	printf("ip is \"%s\"\n",hostName);
	printf("portnum to server is \"%d\"\n",portnum);
	printf("portnum to listen is \"%d\"\n",portnumTel);

	printf("connecting to server\n");
	//make socket---------------------------------------------------------
	sock_server = socket(PF_INET, SOCK_STREAM, 6);
	sock_tel  = socket(PF_INET, SOCK_STREAM, 6);

	return connectAll(portnumTel, hostName, portnum);
}
