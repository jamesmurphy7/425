/**********************************************************************
 * file:  sr_router.c 
 * date:  Mon Feb 18 12:50:42 PST 2002  
 * Contact: casado@stanford.edu 
 *
 * Description:
 * 
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing. 11
 * 90904102
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"

/*--------------------------------------------------------------------- 
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 * 
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr) 
{
    /* REQUIRES */
    assert(sr);

    /* Add initialization code here! */

} /* -- sr_init -- */

/*
	ARPrepresentation
*/

struct sr_ARP_instance
{
	uint16_t HardwareType;
	uint16_t ProtocolType;
	uint8_t  Hlen;
	uint8_t  Plen;
	uint16_t Operation;	

};

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

struct node {
	uint8_t * packet;
	struct node * next;
};

struct ipNode {
	unsigned char   ar_sha[ETHER_ADDR_LEN];   /* mac   */
	uint32_t        ar_sip; /* ip  */
	struct node * next;
};

struct node * packetHead;
struct node * packetTail;
struct ipNode * ipCacheHead;
struct ipNode * ipCacheTail;

void addPacketToCache(uint8_t * packet){
	
	if(packetHead == NULL){
		printf("got here 1\n");
		fflush(stdout);
		struct node * newNode = malloc(sizeof(struct node));
		printf("got here 2\n");
		fflush(stdout);
		newNode->packet = packet;
		newNode->next = NULL;
		packetHead = newNode;
		packetTail = newNode;
	}
	else{
		printf("got here 3\n");
		fflush(stdout);
		struct node * newNode = malloc(sizeof(struct node));
		newNode->packet = packet;
		newNode->next = NULL;
		packetTail->next = newNode;
		packetTail = newNode;
	}
	printf("added packet\n");
	fflush(stdout);
		
}

void sr_handleARP(struct sr_instance* sr, 
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */,
	uint8_t newSourceAddr[ETHER_ADDR_LEN],
	struct sr_ethernet_hdr * ethernethdr)
{
	struct sr_arphdr * arpHeader = (struct sr_arphdr *) (packet + sizeof(struct sr_ethernet_hdr));

	if(htons(arpHeader->ar_op) == ARP_REPLY){
		printf("arp is a reply\n");
		fflush(stdout);
	} 

	if(htons(arpHeader->ar_op) == ARP_REQUEST){
		printf("arp is a request\n");
		fflush(stdout);
		//copy shost and dhost addresses to aprHeader
		for(int i = 0; i < ETHER_ADDR_LEN; i++){
			arpHeader->ar_tha[i] = (unsigned char) ethernethdr->ether_dhost[i];
			arpHeader->ar_sha[i] = (unsigned char) newSourceAddr[i];

			//uint8_t tempByte = ethernethdr->ether_dhost[i];
			ethernethdr->ether_dhost[i] = ethernethdr->ether_shost[i];
			ethernethdr->ether_shost[i] = newSourceAddr[i];		

		}

		arpHeader->ar_op = ARP_REPLY;
		uint32_t tempSip = arpHeader->ar_sip;
		arpHeader->ar_sip = arpHeader->ar_tip;
		arpHeader->ar_tip = tempSip;

		//struct sr_ethernet_hdr * checketh = (struct sr_ethernet_hdr *) packet;
	
		int sendReturnVal = sr_send_packet(sr, packet, 42, interface);
		printf("send_packed returned: %d\n", sendReturnVal);
		fflush(stdout);
		addPacketToCache(packet);
	}	

}

void sr_handlepacket(struct sr_instance* sr, 
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */)
{
    /* REQUIRES */
    assert(sr);
    assert(packet);
    assert(interface);

    printf("*** -> Received packet of length %d \n",len);
	struct sr_ethernet_hdr * ethernethdr = (struct sr_ethernet_hdr *) packet;
	struct sr_if* ifList = sr->if_list;

	char interfacename[32];
	char* tempi = interface;
	for(int i = 0; i < 32; i++){
		interfacename[i] = *tempi;
		printf("%c", interfacename[i]);
		tempi += sizeof(char);
	}
	printf("\n");
	printf("size of name is: %d\n", strlen(interfacename));
	fflush(stdout);
	uint8_t newSourceAddr[ETHER_ADDR_LEN];
	int isFound = 0;
	while(isFound == 0){
		int diff = 0;
		char* interfaceName = interface;
		diff = strcmp(interfacename, ifList->name);
		
		
		if(diff == 0){
			//printf("got here\n");
			//printf("name is |%s|\n", ifList->name);
			fflush(stdout);
			isFound = 1;
			for(int i = 0; i < ETHER_ADDR_LEN; i++){
				newSourceAddr[i] = (uint8_t) ifList->addr[i];
			}
			//printf("got here2\n");
			break;
		}
		ifList = ifList + sizeof(struct sr_if);
	}

	//------------------------------------------------------------------------
	printf("check\n");
	fflush(stdout);
	if(htons(ethernethdr->ether_type) == ETHERTYPE_ARP){
		printf("srhandle arp\n");
		fflush(stdout);
		sr_handleARP(sr, packet, len, interface, newSourceAddr,ethernethdr); 
	}
	else if(htons(ethernethdr->ether_type) == ETHERTYPE_IP){
		printf("srhandle ip\n");
		fflush(stdout);
	}
	else if(htons(ethernethdr->ether_type) == IPPROTO_ICMP){
		printf("srhandle icmp\n");
		fflush(stdout);
	}
	
}/* end sr_ForwardPacket */


/*--------------------------------------------------------------------- 
 * Method:
 *
 *---------------------------------------------------------------------*/
