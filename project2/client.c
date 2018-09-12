
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

int main(int argc, char* argv[]){
	//error checking
	if(argc != 3){
		fprintf(stderr, "not correct number of commandline args\n");
		return 1;
	}
	char* hostName = "";
	hostName = argv[1];
	int portnum;
	char* badString;
	//get portnum
	portnum = strtol(argv[2], &badString, 0);
	if(*badString == '\0'){
		//printf("port num is: %d\n", portnum);
	}
	//error checking
	else{
		fprintf(stderr, "bad portnum\n");
		return 1;
	}


	printf("port num is \"%d\"\n",portnum);


	//make socket
	int sock_desc;
	sock_desc = socket(PF_INET, SOCK_STREAM, 6);

	struct sockaddr_in sin;
	sin.sin_family = PF_INET;
	printf("ip is \"%s\"\n",hostName);
	//copy ip address
	in_addr_t address;
	address = inet_addr(hostName);
	if( address == -1){
		fprintf(stderr, "error setting ip adress\n");
		return 1;
	}
	memcpy(&sin.sin_addr, &address, sizeof(address));
	//memcpy (&sin.sin_addr, hptr->h_addr, hptr->h_length); 
	//set portnum
	sin.sin_port = htons(portnum);

	//connect
	int connectError = connect(sock_desc, (struct sockaddr *) &sin, sizeof(sin));
	if(connectError < 0){
		fprintf(stderr, "could not connect to %s\n", hostName);
		return 1;
	}

	printf("successfully connected to server\n");


	//write to server
	/*quoteNum = htonl(quoteNudm);
	int result = write(sock_desc, &quoteNum, 4);
	if(result < 0 ){
		fprintf(stderr, "could not write to %s\n", hostName);
		return 1;
	}*/
	
	return 0;
}
