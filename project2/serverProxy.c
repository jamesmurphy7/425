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
#include <time.h>

int sock_client;//to client socket
int sock_telnetd;//telnet daemon socket
int sock_listen;//listening socket

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
int multipleListen() {
	fd_set listen;           /* bit mask for listening on socket */        
	struct timeval timeout;  /* timeout for select call */       
	int nfound;              /* number of pending requests that select() found */     
	char helper[1000];
	int deadCounter = 0;
	int waitReconnect = 0;
	clock_t before = clock();

	while(1) {
		if(waitReconnect == 0) {
			//HEARTBEAT TO CLIENT EVERY SECOND
			clock_t difference = clock() - before;
			int msec = difference * 5000 / CLOCKS_PER_SEC;
			if(msec >= 1){
				//printf("ping\n"); 
				char heartBeat[1000] = "ping";
				heartBeat[4] = '\0';
				int heartBeatLen = 4;
				int writeMsg = write(sock_client, heartBeat, heartBeatLen);
				
				if(writeMsg < 0){
					fprintf(stderr, "unable to ping the client\n");
					exit(1);
				}
				before = clock();
			}
		}
		//HANDLE CLOSED CONNECTION IF THREE HEARTBEATS NOT DELIVERED
		if(deadCounter >= 3 && waitReconnect == 0){
			printf("The connection is DEAD\n");
			fflush(stdout);

			//close old client socket
			int closeError = close(sock_client);
			if(closeError == -1){
				fprintf(stderr, "error closing socket\n");
				fflush(stdout);
			}
			printf("closed socket\n");
			fflush(stdout);
			printf("Listening for new connection\n");
			//CHANGE CLIENT BACK TO LISTENING SOCKET
			sock_client = sock_listen;
			waitReconnect = 1;
			deadCounter = 0;
			
		}

		/* need to wait for a message or a timeout */        
		FD_ZERO(&listen);                   /* zero the bit map */        
		FD_SET(sock_telnetd, &listen); /* telnetd socket fdset */    
		FD_SET(sock_client, &listen); /* client socket fdset */  

	     /* set seconds + micro-seconds of timeout */        
		timeout.tv_sec = 1;        
		timeout.tv_usec = 0;


		nfound = select(FD_SETSIZE, &listen, (fd_set *)0, (fd_set *)0, &timeout);

		if (nfound == 0 && waitReconnect == 0) { /* NO COMMUNICATION FOR 1 SECOND*/
			printf("Timeout\n");
			deadCounter++;      
		} 
		else if (nfound < 0) {            
		/* handle error here... */  
			printf("select didnt work\n");   
			return 1;   
		}
		
		//READ FROM CLIENT
		if(FD_ISSET(sock_client, &listen)){ /*message from clientproxy*/
			//CLIENT POINTS TO LISTENING SOCKET
			//TRYING TO RECONNECT
			if(waitReconnect == 1){
				//accept a new connection when it is received
				struct sockaddr_in client;
				size_t size = sizeof(client);
				int accepted = accept(sock_client, (struct sockaddr *) &client, (socklen_t *) &size);
				printf("Accepted!\n");
				if(accepted < 0){
					printf("Accept did not work\n");
					return 1;
				}
				sock_client = accepted;
				waitReconnect = 0;
			//READ AND WRITE
			} else {
				int getter = read(sock_client, helper, 1000);
				
				if(getter == 0){
					printf("getter was 0 test");
					exit(0);
				}
				helper[getter] = '\0';

				if(getter == 4 && strcmp(helper,"ping") == 0){/* then this is a heartbeat message from client*/
					printf("ping from client\n");
					fflush(stdout);
				}
				else { /* else do a normal write to telnet daemon */
					int writeMsg = write(sock_telnetd, helper, getter);
					if(writeMsg < 0){
						fprintf(stderr, "unable to write to telnet daemon\n");
						exit(1);
					}
				}
			}
			deadCounter = 0;
		} 
		//WRITE
		//WRITE TO THE CLIENT WITH TELNET DATA
		if(FD_ISSET(sock_telnetd, &listen)){
			int getter = read(sock_telnetd, helper, 1000);
			
			if(getter == 0){
				printf("Telnet socket read 0 bits\n");
				exit(0);
			}
			
			//printf("Telnet bytes read: %d\n", getter);
			helper[getter] = '\0';
			int writeMsg = write(sock_client, helper, getter);
			if(writeMsg < 0){
				fprintf(stderr, "unable to write telnet data to client\n");
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

	int optval = 1;
    setsockopt(sock_listen, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(sock_listen));

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

	int result = bind(sock_listen, (struct sockaddr *)&sin_toClient, sizeof(sin_toClient));
	if (result < 0){
		fprintf(stderr,"error binding socket for client\n");
		return 1;
	}	

	//listen to port
	result = listen(sock_listen, 10);
	if(result < 0){
		fprintf(stderr, "Error listening to socket.\n");
		return 1;
	}
	//accept a new connection when it is received
	struct sockaddr_in client;
	size_t size = sizeof(client);
	int accepted = accept(sock_listen, (struct sockaddr *) &client, (socklen_t *) &size);
	if(accepted < 0){
		//printf("Could not accept client.\n");
		return 1;
	}

	if(openTelnet() > 0){
		//printf("Could not start telnet\n");
		return 1;
	}
	sock_client = accepted;
	//printf("Connected to telnet, listening to multiple sockets.\n");
	return multipleListen();

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
	sock_listen = socket(PF_INET, SOCK_STREAM, 6);
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
