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
#include <time.h>
#include <errno.h>

int sock_tel;
int sock_server;

int oldPortNum;
char* serverHostName;

int connectToServer(char* hostName, int portnum, int sockToConnect);

//listens to multiple ports using select, forwards data from one
//port to the next.
int multipleListen(int tel_socket) {
	fd_set listen;           /* bit mask for listening on socket */        
	struct timeval timeout;  /* timeout for select call */       
	int nfound;              /* number of pending requests that select() found */     
	char helper[1000];
	clock_t before = clock();
	int deadCounter = 0;
	//printf("listen() socket_server : %d\n", sock_server);
	//printf("listen() socket_tel: %d\n", tel_socket);
	while(1) {


		//ping message every second
		clock_t difference = clock() - before;
		int msec = difference * 5000 / CLOCKS_PER_SEC;
		if(msec >= 1){
			//printf("ping\n"); 
			char heartBeat[1000] = "ping";
			heartBeat[4] = '\0';
			int heartBeatLen = 4;
			int writeMsg = write(sock_server, heartBeat, heartBeatLen);
			
			if(writeMsg < 0){
				fprintf(stderr, "unable to write to server\n");
				exit(1);
			}
			before = clock();
		}
		
		/* need to wait for a message or a timeout */        
		FD_ZERO(&listen);                   /* zero the bit map */        
		FD_SET(tel_socket, &listen); /* telnet socket fdset */    
		FD_SET(sock_server, &listen); /* server socket fdset */  
	     /* set seconds + micro-seconds of timeout */        
		timeout.tv_sec = 1;        
		timeout.tv_usec = 0;

		nfound = select(FD_SETSIZE, &listen, (fd_set *)0, (fd_set *)0, &timeout);

		if(deadCounter >= 3){
			//close socket
			shutdown(sock_server, SHUT_RDWR);
			int closeError = close(sock_server);
			if(closeError == -1){
				fprintf(stderr, "error closing socket\n");
				fflush(stdout);
			}
			printf("closed socket\n");
			fflush(stdout);
			//create new socket
			printf("sock_server before: %d",sock_server);
			sock_server = socket(PF_INET, SOCK_STREAM, 6);
			//reconnect to server
			printf("Attempting to reconnect to the server.\n");
			connectToServer(serverHostName,oldPortNum, sock_server);
			printf("Reconnect successful.\n");
		}

		if (nfound == 0) {/* HAVE NOT READ ANYTHING FOR ONE SECOND*/
			printf("timeout\n");
			fflush(stdout);
			deadCounter++;
		} 
		else if (nfound < 0) {            
		/* handle error here... */  
			//printf("select didnt work\n");   
			return 1;   
		}



		//READ FROM TELNET
		if(FD_ISSET(tel_socket, &listen)){
			int getter = read(tel_socket, helper, 1000);
			helper[getter] = '\0';
			int writeMsg = write(sock_server, helper, getter);
			if(writeMsg < 0){
				printf("unable to write to server\n");
				//exit(1);
				connectToServer(serverHostName,oldPortNum, sock_server);
			}
			if(getter == 0){
				printf("getter is 0 from reading telnet");
				fflush(stdout);
				exit(0);
			}
			deadCounter = 0;
		}

		//READ FROM SERVER      
		if(FD_ISSET(sock_server, &listen)){
			int getter = read(sock_server, helper, 1000);
			//printf("Telnet bytes read: %d\n", getter);
			helper[getter] = '\0';


			//PING FROM SERVER
			if(getter == 4 && strcmp(helper,"ping") == 0){
				printf("ping from server\n");
				fflush(stdout);
			}
			//TELNET DATA FROM SERVER
			else {
				int writeMsg = write(tel_socket, helper, getter);
				if(writeMsg < 0){
					fprintf(stderr, "unable to write to client\n");
					printf("|%s|\n",helper);
					//connectToServer(serverHostName,oldPortNum);
					//exit(1);
				}
				if(getter == 0){
					printf("getter is 0\n");
					fflush(stdout);
					exit(-3);
				}
			}
			deadCounter = 0;
		}
	}
	return 0;

}
/*
 * Handles connecting to server
*/
int connectToServer(char* hostName, int portnum, int sockToConnect){
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
	printf("connectToServer(): socket: %d host: %s port: %d\n", sockToConnect, hostName, portnum);
	if(connect(sockToConnect, (struct sockaddr *) &sin, sizeof(sin)) < 0){
		fprintf(stderr, "could not connect to %s with errno %d\n", hostName, errno);
		exit(1);
	}

	printf("successfully connected to server\n");
	fflush(stdout);
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

	//printf("binding socket...\n");
	int result = bind(sock_tel, (struct sockaddr *)&sin_toTel, sizeof(sin_toTel));
	if(result < 0){
		fprintf(stderr, "error binding socket to Telnet with %d\n", portnum);
		exit(1);
	}

	//printf("listening for a connection...\n");
	result = listen(sock_tel, 10);
	if(result < 0){
		fprintf(stderr, "error connecting to telnet\n");
		exit(1);
	}

	//printf("got a connection, waiting for connection to be accepted...\n");
	struct sockaddr_in client;
	size_t size = sizeof(client);
	int accepted = accept(sock_tel, (struct sockaddr *) &client, (socklen_t *) &size);
	if(accepted < 0){
		//printf("Could not accept telnet\n");
		exit(1);
	}
	//printf("accepted connection from telnet\n");
	//connect to server proxy after
	if(connectToServer(hostname, port, sock_server) != 0){
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

	oldPortNum = portnum;
	serverHostName = hostName;

	//printf("ip is \"%s\"\n",hostName);
	//printf("portnum to server is \"%d\"\n",portnum);
	//printf("portnum to listen is \"%d\"\n",portnumTel);

	//printf("connecting to server\n");
	//make socket---------------------------------------------------------
	sock_server = socket(PF_INET, SOCK_STREAM, 6);
	sock_tel  = socket(PF_INET, SOCK_STREAM, 6);

	return connectAll(portnumTel, hostName, portnum);
}