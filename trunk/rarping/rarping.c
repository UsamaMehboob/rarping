/**
 * @file rarping.c
 *
 * @brief Rarping - send RARP REQUEST to a neighbour host

 *
 * $Author: $
 * $Date: $
 *
 * $Revision: $
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


signed char argumentManagement ( long l_argc, char **ppch_argv, opt_t *pst_argsDest )
{
	signed char c_retValue;
	char ch_opt;

	/* Initialisation */
	c_retValue = 1;
	pst_argsDest->pch_iface = NULL;
	pst_argsDest->pch_askedHwAddr = NULL;
	/* ************** */


	/* Parsing options args */
	while ( ( ch_opt = getopt( l_argc, ppch_argv, "I:" ) ) != -1 ) 
	{
		switch(ch_opt)
		{
			case 'I'	:	pst_argsDest->pch_iface = optarg;
							break;

			case 'h'	:
			case '?'	:
			default		:	c_retValue = -1;
		}
	}
	
	/* parsing non options args */
	/* The only one must be the MAC Addr we'll request related IP */
	if (optind < l_argc)
		pst_argsDest->pch_askedHwAddr = ppch_argv[optind];
	else
		c_retValue = -1;

	/* Check if required infos had been given */
	if ( (pst_argsDest->pch_iface == NULL) || (pst_argsDest->pch_askedHwAddr == NULL) )
		c_retValue = -1;
	else
	{
		/* nothing to do */
	}

#ifdef DEBUG
	fprintf(stderr, "Iface : %s\n", pst_argsDest->pch_iface);
	fprintf(stderr, "Request about : %s\n", pst_argsDest->pch_askedHwAddr);
#endif

	return c_retValue;
}


void usage ( void )
{
	printf("\n-=] Rarp requests sender [=-\n");
	printf("           ****          \n");
	printf("\nVersion : %s\n", VERSION);
	printf("Usage : ./rarping -I [interface] [request MAC address]\n");
	printf("For example : ./rarping -I eth0 00:03:13:37:be:ef\n\n");

	return;
}


signed char performRequests ( const opt_t *pst_argsDest )
{
	long l_socket, nbProbes;
	signed char c_retValue;
	etherPacket_t str_packet;
	struct sockaddr_ll str_device; /* device independant physical layer address */

	c_retValue = 0;
	nbProbes = 0;

	if ((l_socket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) /* socket opening */
	{
		perror("socket");
		if (!IS_ROOT)
			fprintf(stderr, "Are you root?\n");
		c_retValue = -1;
	}
	else
	{
		if ( craftPacket(&str_packet, pst_argsDest->pch_iface, &str_device, pst_argsDest->pch_askedHwAddr, l_socket) == 0 )
		{
			while (++nbProbes < MAX_PROBES);
				if (sendProbe(l_socket, &str_packet, &str_device) != 1)
				{
					fprintf(stderr, "Can't send request #%ld\n", nbProbes);
				}
				else
				{
#ifdef DEBUG
					fprintf(stderr, "Request #%ld sent\n", nbProbes);
#endif
				}
		}
		else
		{
			fprintf(stderr, "Can't craft packets\n");
			c_retValue = -2;
		}
	
		close(l_socket);
	}
	
	return c_retValue;
}


signed char craftPacket ( etherPacket_t * pstr_packet, unsigned char * pch_ifaceName, struct sockaddr_ll * pstr_device, const unsigned char * pch_askedHwAddr, long l_socket )
{
	signed char c_retValue;
/*	struct sockaddr_ll str_device; * device independant physical layer address */

	c_retValue = 0;
	if ( getLowLevelInfos(pstr_device, pch_ifaceName, l_socket) < 0 )
	{
		fprintf(stderr, "Critical : can't access device level on %s\n", pch_ifaceName);
		c_retValue = -1;
	}
	else
	{
		/* Craft Packet */
		memset(pstr_packet->ucht_destHwAddr, 0xFF, 6);
		memcpy(pstr_packet->ucht_senderHwAddr, pstr_device->sll_addr, 6);
		pstr_packet->us_ethType = htons(ETH_TYPE_RARP);
		pstr_packet->str_packet.us_hwType = htons(0x01); /* #define ETHERNET 0x01*/
		pstr_packet->str_packet.us_protoType = htons(0x800); /* #define IP_PROTO 0x800 */
		pstr_packet->str_packet.uc_hwLen = 6; /* length of mac address in bytes */
		pstr_packet->str_packet.uc_protoLen = 4; /* Were're in IPV4 here */
		pstr_packet->str_packet.us_opcode = htons(0x03); /* #define request/reply 3/4 */
		memcpy(pstr_packet->str_packet.cht_srcHwAddr, pstr_device->sll_addr, 6);
		/*
		 * TODO : fill this field with our IP address or whatever the user wants
		 */
		bzero(pstr_packet->str_packet.cht_srcIpAddr, 4);
		bzero(pstr_packet->str_packet.cht_targetIpAddr, 4);

#define MAC_FIELD(a) &(pstr_packet->str_packet.cht_targetHwAddr[(a)])
		if (sscanf(pch_askedHwAddr, "%02x:%02x:%02x:%02x:%02x:%02x", MAC_FIELD(0), MAC_FIELD(1), MAC_FIELD(2), MAC_FIELD(3), MAC_FIELD(4), MAC_FIELD(5)) != 6)
		{
			fprintf(stderr, "Unrecognised format %s for a MAC address\n", pch_askedHwAddr);
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


char getLowLevelInfos ( struct sockaddr_ll * pstr_device, unsigned char * pch_ifaceName, long l_socket )
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


char getLocalHardwareAddress ( long l_socket, unsigned char * pch_ifaceName, unsigned char * mac )
{
	char c_retValue;
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
#ifdef DEBUG
	#define MAC(i) (str_tmpIfr.ifr_hwaddr.sa_data[(i)])
		fprintf(stderr, "Local MAC address is %02x:%02x:%02x:%02x:%02x:%02x\n", MAC(0), MAC(1), MAC(2), MAC(3), MAC(4), MAC(5));
#endif
		memcpy(mac, str_tmpIfr.ifr_hwaddr.sa_data, 6); /* cpoy the local MAC address into mac buffer (into sockaddr_sll str_device in fact)*/
	}

	return c_retValue;
}


unsigned long getIfaceIndex ( unsigned char * pch_ifName, long l_socket )
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
#ifdef DEBUG
		fprintf(stderr, "Selected interface's Index is %d\n",str_tmpIfr.ifr_ifindex);
#endif
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

