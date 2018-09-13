
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

int sock_desc;

int waitForClient(int sock) {
	int len;
	int readLine;
	//working
	while(1) {
		readLine = read(sock, &len, sizeof(len));
		//closed connection
		if( readLine <= 0) {
			break;
		}
		len = ntohl(len);
		printf("size received: %d\n", len);
		char msg[len];
		readLine = read(sock, &msg, len);
		if( readLine <= 0) {
			break;
		}
		msg[len] = '\0';
		printf("Message: %s\n", msg);
		
	}
	printf("Connection has been closed. Exiting...\n");
	close(sock);
	return 0;
}

int startServer(int portnum) {

	struct sockaddr_in sin;
	sin.sin_family = PF_INET;
	//htons(portnum);
	sin.sin_port = htons(portnum);
	sin.sin_addr.s_addr = INADDR_ANY;

	int result = bind(sock_desc, (struct sockaddr *)&sin, sizeof(sin));

	if (result < 0){
		fprintf(stderr,"error binding socket\n");
		return 1;
	}

	printf("listening for a connection\n");
	result = listen(sock_desc, 10);
	if(result < 0){
		printf("Error listening to socket.\n");
		return 1;
	}


	//accept a new connection when it is received
	struct sockaddr_in client;
	size_t size = sizeof(client);
	int accepted = accept(sock_desc, (struct sockaddr *) &client, (socklen_t *) &size);
	printf("connection received\n");

	waitForClient(accepted);



	return 0;

}

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

	//bind socket
	sock_desc = socket(PF_INET, SOCK_STREAM, 6);
	if(sock_desc < 0){
		fprintf(stderr, "bad socket_desc\n");
		return 1;
	}

	return startServer(portnum);

}