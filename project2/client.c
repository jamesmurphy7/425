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

int sock_desc;
int sock_tel;
int sock_client;

int writeLoop(){
	//write to server
	//test print
	char* toWriteStr = "";
	size_t bufsize = 1024;
	toWriteStr = (char*)malloc(bufsize * sizeof(char));
	if(toWriteStr == NULL){
		fprintf(stderr, "error\n");
		return 1;
	}
	int toWriteLen;
	//write to server
	while(getline(&toWriteStr,&bufsize,stdin) != EOF) {

		toWriteLen = strlen(toWriteStr);
		//delete extra newline on toWriteStr
		
		toWriteStr[toWriteLen -1] = '\0';
		
		toWriteLen = htonl(toWriteLen);

		int writeMsg = write(sock_desc, &toWriteLen, sizeof(toWriteLen));

		if(writeMsg < 0){
			fprintf(stderr, "Unable to write to server\n");
			return 1;
		}
		//send string
		writeMsg = write(sock_desc, toWriteStr, strlen(toWriteStr)+1);

		if(writeMsg < 0){
			fprintf(stderr, "Unable to write to server\n");
			return 1;
		}
	}
	return 0;
}

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
	int connectError = connect(sock_desc, (struct sockaddr *) &sin, sizeof(sin));
	if(connectError < 0){
		fprintf(stderr, "could not connect to %s\n", hostName);
		return 1;
	}

	printf("successfully connected to server\n");
	
	return 0;
}

//listens to multiple ports using select
int multipleListen(int client_socket) {
	fd_set listen;           /* bit mask for listening on socket */        
	struct timeval timeout;  /* timeout for select call */       
	int nfound;              /* number of pending requests that select() found */     
	char helper[1000];
	printf("listen() socket_client : %d\n", sock_desc);
	printf("listen() socket_telnet: %d\n", client_socket);
	while(1) {
		/* need to wait for a message or a timeout */        
		FD_ZERO(&listen);                   /* zero the bit map */        
		FD_SET(client_socket, &listen); /* client socket fdset */    
		FD_SET(sock_desc, &listen); /* telnet socket fdset */  
	     /* set seconds + micro-seconds of timeout */        
		timeout.tv_sec = 20;        
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
		if(FD_ISSET(sock_desc, &listen)){
			printf("Message from telnet: \n");
			int getter = recv(sock_desc, helper, 1000, 0);
			helper[getter] = '\0';
			printf("%s", helper);

			int writeMsg = write(client_socket, helper, strlen(helper));
			if(writeMsg < 0){
				fprintf(stderr, "unable to write to server\n");
				exit(1);
			}
		}

		if(FD_ISSET(client_socket, &listen)){
			printf("Message from client: \n");
			int getter = read(client_socket, helper, 1000);
			helper[getter] = '\0';
			printf("%s", helper);

			int writeMsg = write(sock_desc, helper, strlen(helper));
			if(writeMsg < 0){
				fprintf(stderr, "unable to write to server\n");
				exit(1);
			}
		}
	}
	return 0;

}

int connectToTel(int portnum){
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
	printf("accepted connection to telnet\n");
	sock_client = accepted;
	multipleListen(accepted);
	return 0;
}

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
	sock_desc = socket(PF_INET, SOCK_STREAM, 6);
	sock_tel  = socket(PF_INET, SOCK_STREAM, 6);

	if(connectToServer(hostName, portnum) != 0){
		return 1;
	}

	connectToTel(portnumTel);

	return writeLoop();
}
