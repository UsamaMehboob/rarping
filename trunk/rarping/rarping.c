/**
 * @file rarping.c
 *
 * @brief Rarping - send RARP REQUEST to a neighbour host
 * @see RFC 903
 *
 * $Author: Henri Doreau <henri.doreau@gmail.com> $
 * $Date: dimanche 6 avril 2008, 21:17:46 (UTC+0200) $
 *
 * $Revision: 10 $
 */

/* 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include "rarping.h"


int main ( int i_argc, char **ppch_argv )
{
    /* Return value */
    signed char c_retValue;
	long l_argc;
	opt_t str_args;

	l_argc = i_argc; /* if sizeof(int) != sizeof(long) */
    c_retValue = 0;

    /* Parse args using getopt to fill a struct (args), return < 0 if problem encountered */
    if ( argumentManagement(l_argc, ppch_argv, &str_args) > 0 )
    {
        /* perform RARP requests as wanted by user (using args struct) */
		c_retValue = performRequests(&str_args);
    }
    else
    {
        /* If any problem occured, explain the user how to build his command line */
        usage();
        /* and exit < 0 */
        c_retValue = -1;
    }


    /* Simple exit point */
    return c_retValue;
}


signed char argumentManagement ( long l_argc, char **ppch_argv, opt_t *pstr_argsDest )
{
	signed char c_retValue;
	char ch_opt;

	/* Initialisation */
	c_retValue = 1;
	pstr_argsDest->pch_iface = NULL;
	pstr_argsDest->pch_askedHwAddr = NULL;
	pstr_argsDest->ul_count = 0;
	pstr_argsDest->uc_choosenOpCode = RARP_OPCODE_REQUEST;
	/* ************** */


	/* Parsing options args */
	while ( ( ch_opt = getopt( l_argc, ppch_argv, "I:c:Vha" ) ) != -1 ) 
	{
		switch(ch_opt)
		{
			case 'I'	:	pstr_argsDest->pch_iface = optarg;
							break;

			case 'c'	:	pstr_argsDest->ul_count = ABS(atol(optarg)); /* < 0 were stupid */
							if (pstr_argsDest->ul_count == 0)
								c_retValue = -1;
							break;

			case 'a'	:	pstr_argsDest->uc_choosenOpCode = RARP_OPCODE_REPLY;
							break;

			case 'V'	:	fprintf(stdout, "%s\n", VERSION);
							exit(1);
							break;
			case 'h'	:
			case '?'	:
			default		:	c_retValue = -1;
		}
	}
	
	/* parsing non options args */
	/* The only one must be the MAC Addr we'll request related IP */
	if (optind < l_argc)
		pstr_argsDest->pch_askedHwAddr = ppch_argv[optind];
	else
		c_retValue = -1;

	/* Check if required infos had been given */
	if ( (pstr_argsDest->pch_iface == NULL) || (pstr_argsDest->pch_askedHwAddr == NULL) )
		c_retValue = -1;
	else
	{
		fprintf(stdout, "RARPING %s on %s\n", pstr_argsDest->pch_askedHwAddr, pstr_argsDest->pch_iface);
	}

	return c_retValue;
}


void usage ( void )
{
	fprintf(stderr, "Usage : ./rarping [-h] [-c count] [-I interface] request_MAC_address\n");
	fprintf(stderr, "\t-h : print this screen and exit\n");
	fprintf(stderr, "\t-V : print version and exit\n");
	fprintf(stderr, "\t-c count : send [count] request(s) and exit\n");
	fprintf(stderr, "\t-a : send replies instead of requests\n");
	fprintf(stderr, "\t-I interface : network device to use\n");
	fprintf(stderr, "\trequest_MAC_address : hardware address we request associated IP address\n");
	fprintf(stderr, "For example : ./rarping -I eth0 00:03:13:37:be:ef\n");

	return;
}


signed char performRequests ( const opt_t *pstr_argsDest )
{
	long l_socket, l_nbProbes, l_receivedReplies;
	signed char c_retValue;
	etherPacket_t str_packet;
	struct sockaddr_ll str_device; /* device independant physical layer address */

	c_retValue = 0;
	l_nbProbes = 0;
	l_receivedReplies = 0;

	if ((l_socket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) /* opening socket */
	{
		perror("socket");
		if (!IS_ROOT)
			fprintf(stderr, "Are you root?\n");
		c_retValue = -1;
	}
	else
	{
		if ( craftPacket(&str_packet, pstr_argsDest, &str_device, l_socket) == 0 )
		{
			while ( (++l_nbProbes <= pstr_argsDest->ul_count) || (!(pstr_argsDest->ul_count)) )/* infinite loop if no count specified */
				if (sendProbe(l_socket, &str_packet, &str_device) != 1)
				{
					fprintf(stderr, "Can't send request #%ld\n", l_nbProbes);
				}
				else
				{
#ifdef DEBUG
					fprintf(stderr, "Request #%ld sent\n", l_nbProbes);
#endif
					/* getAnswer function returns the number of replies received */
					l_receivedReplies += getAnswer(l_socket, &str_device); /* wait for an answer and parse it */
				}
		}
		else
		{
			fprintf(stderr, "Can't craft packets\n");
			c_retValue = -2;
		}
		/* Print out results if at least one request sent */
		if (l_nbProbes > 0)
		{
			fprintf(stdout, "Sent %ld request(s)\n", l_nbProbes-1);
			fprintf(stdout, "Received %ld response(s)\n", l_receivedReplies);
		}
		else
		{
			/* nothing to do */
		}
		close(l_socket);
	}

	return c_retValue;
}


signed char craftPacket ( etherPacket_t * pstr_packet, const opt_t * pstr_destArgs, struct sockaddr_ll * pstr_device, long l_socket )
{
	signed char c_retValue;

	c_retValue = 0;


	if ( getLowLevelInfos(pstr_device, pstr_destArgs->pch_iface, l_socket) < 0 )
	{
		fprintf(stderr, "Critical : can't access device level on %s\n", pstr_destArgs->pch_iface);
		c_retValue = -1;
	}
	else
	{
		/* Craft Packet */
		memset(pstr_packet->uct_destHwAddr, 0xFF, 6);
		memcpy(pstr_packet->uct_senderHwAddr, pstr_device->sll_addr, 6);
		pstr_packet->us_ethType = htons(ETH_TYPE_RARP);
		pstr_packet->str_packet.us_hwType = htons(HW_TYPE_ETHERNET);
		pstr_packet->str_packet.us_protoType = htons(IP_PROTO);
		pstr_packet->str_packet.uc_hwLen = 6; /* length of mac address in bytes */
		pstr_packet->str_packet.uc_protoLen = 4; /* Were're in IPV4 here */
		pstr_packet->str_packet.us_opcode = htons(pstr_destArgs->uc_choosenOpCode);
		memcpy(pstr_packet->str_packet.uct_srcHwAddr, pstr_device->sll_addr, 6);
		/* In a RARP request these fields are undefined */
		bzero(pstr_packet->str_packet.uct_srcIpAddr, 4);
		bzero(pstr_packet->str_packet.uct_targetIpAddr, 4);
		/* --- -- --- -- --- -- --- -- --- -- --- -- -- */
#define MAC_FIELD(a) (&(pstr_packet->str_packet.uct_targetHwAddr[(a)]))
		if (sscanf(pstr_destArgs->pch_askedHwAddr, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", MAC_FIELD(0), MAC_FIELD(1), MAC_FIELD(2), MAC_FIELD(3), MAC_FIELD(4), MAC_FIELD(5)) != 6)
		{
			fprintf(stderr, "Unrecognised format %s for a MAC address\n", pstr_destArgs->pch_askedHwAddr);
			fprintf(stderr, "Use : aa:bb:cc:dd:ee:ff notation\n");
			c_retValue = -2;
		}
		else
		{
			/* nothing to do */
		}
	}

	return c_retValue;
}


char getLowLevelInfos ( struct sockaddr_ll * pstr_device, char * pch_ifaceName, long l_socket )
{
	signed char c_retValue;

	/* --- Init --- */
	c_retValue = 0;
	bzero(pstr_device, sizeof(struct sockaddr_ll));
	/* --- -- --- */

	pstr_device->sll_family = AF_PACKET;
	pstr_device->sll_protocol = htons(ETH_P_ALL);
	pstr_device->sll_ifindex = getIfaceIndex(pch_ifaceName, l_socket);
	pstr_device->sll_halen = 6; /* Hardware address length */

	if ( getLocalHardwareAddress(l_socket, pch_ifaceName, pstr_device->sll_addr) < 0 )
	{
		fprintf(stderr, "Can't find local hardware address (MAC address)\n");
		c_retValue = -1;
	}
	else
	{
		/* nothing to do */
	}

	return c_retValue;
}


char getLocalHardwareAddress ( long l_socket, char * pch_ifaceName, unsigned char * puc_mac )
{
	signed char c_retValue;
	struct ifreq str_tmpIfr; /* described in man (7) netdevice */

	c_retValue = 0;
	bzero(&str_tmpIfr, sizeof(struct ifreq));
	strncpy(str_tmpIfr.ifr_name, pch_ifaceName, IFNAMSIZ-1);  /* copy iface name into the ifreq structure we've just declared // IFNAMSIZ defined in net/if.h */

	/* Get local hardware address */

	if (ioctl(l_socket, SIOCGIFHWADDR, &str_tmpIfr) == -1 )
	{
		perror("ioctl");
		c_retValue = -1;
	}
	else
	{
		memcpy(puc_mac, str_tmpIfr.ifr_hwaddr.sa_data, 6); /* cpoy the local MAC address into mac buffer (into struct sockaddr_sll str_device in fact)*/
	}

	return c_retValue;
}


unsigned long getIfaceIndex ( char * pch_ifName, long l_socket )
{
	struct ifreq str_tmpIfr;

	bzero(&str_tmpIfr, sizeof(struct ifreq));
	strncpy(str_tmpIfr.ifr_name, pch_ifName, IF_NAMESIZE-1);

	/* get card Index */
	if( ioctl( l_socket, SIOCGIFINDEX, &str_tmpIfr) < 0 )
	{
		perror("ioctl");
	}
	else
	{
		/* nothing to do */
	}
    
	return str_tmpIfr.ifr_ifindex;
}


signed char sendProbe ( long l_socket, etherPacket_t * pstr_packet, struct sockaddr_ll * pstr_device )
{
	signed char c_retValue;

	c_retValue = 1;

	if (sendto(l_socket, pstr_packet, sizeof(etherPacket_t), 0, (const struct sockaddr *)pstr_device, sizeof(struct sockaddr_ll)) <= 0)
	{
		perror("Sendto");
		c_retValue = -1;
	}
	return c_retValue;
}


unsigned char getAnswer ( long l_socket, struct sockaddr_ll * pstr_device )
{
	etherPacket_t str_reply; /* to store received datas */
	struct in_addr str_replySrcIpAddr; /* to store the IP address of the sender of the replies */
	/* strings to print out results in a clean way */
	char tch_replySrcIp[IP_ADDR_SIZE+1], tch_replySrcHwAddr[MAC_ADDR_SIZE+1], tch_replyHwAddr[MAC_ADDR_SIZE+1], tch_replyAddrIp[IP_ADDR_SIZE+1];
	unsigned char uc_retValue;

	/* usual initialisation */
	uc_retValue = 1;
	bzero(&str_reply, sizeof(etherPacket_t));
	/* strings to print results out */
	bzero(tch_replySrcIp, IP_ADDR_SIZE+1);
	bzero(tch_replySrcHwAddr, MAC_ADDR_SIZE+1);
	bzero(tch_replyHwAddr, MAC_ADDR_SIZE+1);
	bzero(tch_replyAddrIp, IP_ADDR_SIZE+1);
	/* --- -- --- -- --- -- --- -- */
	
#ifdef DEBUG
	fprintf(stderr,"Waiting for an reply...\n");
#endif

	/* Reception */
	if ( recvfrom(l_socket, &str_reply, sizeof(etherPacket_t), 0, (struct sockaddr *)pstr_device, (unsigned int *)sizeof(struct sockaddr_ll)) > 0 )
	{
		/* If received packet is a RARP reply */
		if ( (str_reply.us_ethType == htons(ETH_TYPE_RARP)) && (str_reply.str_packet.us_opcode == htons(RARP_OPCODE_REPLY)) )
		{
			/* we craft strings to print results using received packet */
			parse(&str_reply, tch_replySrcIp, tch_replySrcHwAddr, tch_replyHwAddr, tch_replyAddrIp);
			fprintf(stdout, "Reply received from %s (%s) : %s has %s\n", tch_replySrcIp, tch_replySrcHwAddr, tch_replyHwAddr, tch_replyAddrIp);
		}
	}
	else
	{
		uc_retValue = 0;
	}

	return uc_retValue;
}


char parse ( etherPacket_t * pstr_reply, char tch_replySrcIp[], char tch_replySrcHwAddr[], char tch_replyHwAddr[], char tch_replyAddrIp[] )
{
	struct in_addr str_tmpIpAddr;

	/* Fill the string tch_srcIpAddr, that is the IP address of the reply sender with the address contained in received packet */
	bzero(&str_tmpIpAddr, sizeof(struct in_addr));
	memcpy(&str_tmpIpAddr, pstr_reply->str_packet.uct_srcIpAddr, 4);
	strncpy(tch_replySrcIp, inet_ntoa(str_tmpIpAddr), IP_ADDR_SIZE);

	/* 
	 * Fill the string tch_replyIpAddr, that is the IP address of the host which MAC address is the one we requested about
	 * This is the "real" answer to the request sent
	 */
	bzero(&str_tmpIpAddr, sizeof(struct in_addr));
	memcpy(&str_tmpIpAddr, pstr_reply->str_packet.uct_targetIpAddr, 4);
	strncpy(tch_replyAddrIp, inet_ntoa(str_tmpIpAddr), IP_ADDR_SIZE); /* this is the answer */

	/* 
	 * Fill the string tch_replySrcHwAddr with formatted MAC address contained in received datas
	 * This is the hardware address of the sender of the parsed reply
	 */
#define MAC(i) pstr_reply->str_packet.uct_srcHwAddr[(i)]
	snprintf(tch_replySrcHwAddr, MAC_ADDR_SIZE+1, "%02x:%02x:%02x:%02x:%02x:%02x", MAC(0), MAC(1), MAC(2), MAC(3), MAC(4), MAC(5));

	/* The same way, but with Hardware Address of the host we requested about (this must be the same than the one specified by user)) */
#define _MAC(i) pstr_reply->str_packet.uct_targetHwAddr[(i)]
	snprintf(tch_replyHwAddr, MAC_ADDR_SIZE+1, "%02x:%02x:%02x:%02x:%02x:%02x", _MAC(0), _MAC(1), _MAC(2), _MAC(3), _MAC(4), _MAC(5));
	/* --- -- --- -- --- */

	return 0;
}

