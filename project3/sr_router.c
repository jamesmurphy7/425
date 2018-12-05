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
#include <stdint.h>
#include <sys/types.h>
#include <arpa/inet.h>

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
	unsigned char  * ar_sha;   /* mac   */
	uint32_t ar_sip; /* ip  */
	struct node * next;
};

struct node * packetHead;
struct node * packetTail;
struct ipNode * ipCacheHead;
struct ipNode * ipCacheTail;

unsigned char * getEthernet(uint32_t ar_sip){
	struct ipNode * currNode = ipCacheHead;	
	while(currNode != NULL){
		if(currNode->ar_sip == ar_sip){
			return currNode->ar_sha;
		}
		currNode = currNode->next;
	}
	printf("could not find ethernet given ip\n");
	fflush(stdout);
	return NULL;
}

void addIpToCache(struct sr_arphdr * ArpHeader){
	if(ipCacheHead == NULL){
		struct ipNode * newNode = malloc(sizeof(struct ipNode));
		memcpy(newNode->ar_sha, ArpHeader->ar_sha, ETHER_ADDR_LEN);//works
		newNode->ar_sip = ArpHeader->ar_tip;
		newNode->next = NULL;
		ipCacheHead = newNode;
		ipCacheTail = newNode;
		
		//printf("%c - %c\n", newNode->ar_sha[0], ArpHeader->ar_sha[0]);
		//printf("%c - %c\n", newNode->ar_sha[3], ArpHeader->ar_sha[3]);
		//printf("%c - %c\n", newNode->ar_sha[5], ArpHeader->ar_sha[5]);
	}
	else{
		struct ipNode * newNode = malloc(sizeof(struct ipNode));
		newNode->ar_sha = ArpHeader->ar_sha;
		newNode->ar_sip = ArpHeader->ar_sip;
		newNode->next = NULL;
		ipCacheTail->next = newNode;
		ipCacheTail = newNode;
	}
}


void addPacketToCache(uint8_t * packet){
	
	if(packetHead == NULL){
		//printf("got here 1\n");
		fflush(stdout);
		struct node * newNode = malloc(sizeof(struct node));
		//printf("got here 2\n");
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

void delete(uint8_t * packet){
	struct node * currNode = packetHead;
	struct node * delNode = NULL;
	struct node * prevNode = NULL;
	while(currNode != NULL){
		if(memcmp(packet,currNode->packet) == 0){
			delNode = currNode;
			break;
		}
		currNode = currNode->next;
	}
	//couldn't find node so do nothing
	if(currNode == NULL){
		return;
	}
	//if its the head delete the head
	if(delNode == packetHead){
		packetHead = delNode->next;
		//if its the tail delete the tail
		if(delNode == packetTail){
			packetTail = NULL;
		}
		return;
	}
	//else get prev node
	currNode = packetHead;
	while(currNode != NULL){
		if(memcmp(packet,currNode->next->packet) == 0){
			prevNode = currNode;
			break;
		}
		currNode = currNode->next;
	}
	//set the prevnodes next to delnode next
	prevNode->next = delNode->next;
	//if the del node is tail delete tail
	if(delNode == packetTail){
		packetTail = prevNode;
	}
	return;
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
		addIpToCache(arpHeader);
		fflush(stdout);
	} 

	if(htons(arpHeader->ar_op) == ARP_REQUEST){
		printf("arp is a request\n");
		fflush(stdout);
		//copy shost and dhost addresses to aprHeader
		memcpy(ethernethdr->ether_dhost, ethernethdr->ether_shost, ETHER_ADDR_LEN);
		memcpy(ethernethdr->ether_shost, newSourceAddr, ETHER_ADDR_LEN);
		memcpy(arpHeader->ar_tha, arpHeader->ar_sha, ETHER_ADDR_LEN);
		memcpy(arpHeader->ar_sha, newSourceAddr, ETHER_ADDR_LEN);
		arpHeader->ar_op = htons(ARP_REPLY);
		uint32_t tempSip = arpHeader->ar_sip;
		arpHeader->ar_sip = arpHeader->ar_tip;
		arpHeader->ar_tip = tempSip;
	
		int sendReturnVal = sr_send_packet(sr, packet, 42, interface);
		printf("send_packet returned: %d\n", sendReturnVal);
		fflush(stdout);
		addPacketToCache(packet);
		addIpToCache(arpHeader);
	}	

}
uint16_t isCheckSumValid(void* vdata,size_t length) {
    // Cast the data pointer to one that can be indexed.
    char* data=(char*)vdata;

    // Initialise the accumulator.
    uint32_t acc=0xffff;

    // Handle complete 16-bit blocks.
    for (size_t i=0;i+1<length;i+=2) {
        uint16_t word;
        memcpy(&word,data+i,2);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Handle any partial block at the end of the data.
    if (length&1) {
        uint16_t word=0;
        memcpy(&word,data+length-1,1);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }

    // Return the checksum in network byte order.
    return htons(~acc);
}/* end ip_checksum */

uint16_t setCksum(uint8_t* hdr, int len)
{
    long sum = 0;
    
    while(len > 1)
    {
        sum += *((unsigned short*)hdr);
        printf("sum: %08x\n", sum);
        hdr = hdr + 2;
        if(sum & 0x80000000)
        {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        len -= 2;
    }
    
    if(len)
    {
        sum += (unsigned short) *(unsigned char *)hdr;
    }
    
    while(sum>>16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}/* end ip_checksum */
int send_ARP_Request(struct sr_instance* sr, 
					uint8_t * packet, 
					unsigned int len, 
					char* interface,
					uint8_t newSourceAddr[ETHER_ADDR_LEN],
					struct sr_ethernet_hdr * ethernethdr,
					uint32_t lookingfor) {

	uint8_t * newPacket = malloc(sizeof(sr_ethernet_hdr)+sizeof(sr_arphdr));

	printf("\nSEND ARP REQUEST\n");
	fflush(stdout);

	arpHeader->ar_op = htons(ARP_REQUEST);
	arpHeader->ar_sip = arpHeader->ar_tip;
	arpHeader->ar_tip = lookingfor;
	printf("LOOKING FOR: %d\n", lookingfor);
	printf("SENDING FROM: %d\n", arpHeader->ar_sip);

	int sendReturnVal = sr_send_packet(sr, packet, 42, interface);
	printf("send_packet returned: %d\n", sendReturnVal);
	fflush(stdout);
	return sendReturnVal;

}
void sr_handleIP(struct sr_instance* sr, 
					uint8_t * packet, 
					unsigned int len, 
					char* interface,
					uint8_t newSourceAddr[ETHER_ADDR_LEN],
					struct sr_ethernet_hdr * ethernethdr) {

	assert(sr);

	struct ip* ip_packet = (struct ip*)(packet + sizeof(struct sr_ethernet_hdr));//process data as ip packet

	if(ip_packet->ip_src.s_addr == ip_packet->ip_dst.s_addr){
		printf("sr_handleIP():::Same address. Discarding packet...\n");
		return;
	}

	/* WORKING
	int len_ip = (ip_packet->ip_hl)*2;
	printf("sr_handleIP()::: len : %d\n", len_ip);
	printf("sr_handleIP()::: oldCheck : %d\n", ip_packet->ip_sum);
	uint16_t ttt = ip_packet->ip_sum;
	ip_packet->ip_sum = 0x0000;
	printf("Checksum test: %d\n", setCksum(ip_packet, sizeof(struct ip)));
	//decrement TTL 
	*/

	if(ip_packet->ip_ttl == 0){
		printf("sr_handleIP():::Packet has reached end of life. Discarding packet...\n");
		return;
	}
	//Find next hop
	struct sr_rt * n = sr->routing_table;//next hop pointer into routing table
	struct sr_rt * def = n;
	int start = 1;
	while(n != NULL){
		//dont check the default gateway.
		if(n->dest.s_addr == 0){
			def =  n;
			n = n->next;
			continue;
		}
		//found the correct address from the routing table
		//printf("destination of packet: %d\n", ip_packet->ip_dst.s_addr);
		//BITWISE AND with the mask to get next hop
		struct in_addr ip_where = ip_packet->ip_dst;
		ip_where.s_addr = ip_where.s_addr & n->mask.s_addr;
		//printf("WHERE? : %d\n", ip_where.s_addr);
		if(ip_where.s_addr == n->dest.s_addr){
			printf("Found next hop\n");
			break;
		}
		n = n->next;
	}
	//use the default gateway.
	if(n == NULL){
		n = def;
	}
	//n is next hop
	struct ipNode * list = ipCacheHead;
	int found = 0;
	while(list != NULL){
		printf("Next Hop IP: %d\n", n->dest.s_addr);
		printf("Next Hop interface: %s\n", n->interface);
		printf("LIST IP: %d\n", list->ar_sip);
		printf("LIST MAC: ");
		for(int i = 0; i < ETHER_ADDR_LEN; i++){
			printf("%02x", list->ar_sha[i]);
		}
		printf("\n");
		if(list->ar_sip == n->dest.s_addr){
			//found the hardware address of the next hop
			printf("FOUND HARDWARE\n");
			found = 1;
		}
		list = list->next;
	}
	if(found == 0){
		int res = send_ARP_Request(sr, packet, len, interface, newSourceAddr, ethernethdr, n->dest.s_addr);
	}
	/*
	c.Use the routing table to find the IP address of the next hop. DONE

	d.Check ARP cache for the Ethernet address of the next hop. 
	If needed, it should send an ARP request to get the Ethernet address.

	e.While the router is waiting for the ARP reply, 
	it should buffer incoming packets that go to the same next hop.

	f.After the ARP reply is received, save the information in the ARP cache, 
	and send out all the packets that are waiting for the ARP reply.
	*/
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

    printf("\n\n*** -> Received packet of length %d \n",len);
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
			memcpy(newSourceAddr, ifList->addr, ETHER_ADDR_LEN);
			printf("MY IP: %d\n", ifList->ip);
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
		sr_handleIP(sr, packet, len, interface, newSourceAddr, ethernethdr);
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
