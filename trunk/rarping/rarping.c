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

#include <stdio.h>
#include <stdlib.h> /* malloc() */
#include <string.h> /* strncpy(), memcpy() ... */
#include <unistd.h> /* getopt() : command line parsing */
#include <errno.h>
#include <sys/socket.h> /* Network */
#include <sys/types.h> /* for old systems */
#include <arpa/inet.h> /* htons() ... */ 
#include <linux/if_ether.h> /* ETH_P_ALL */
#include <sys/ioctl.h> /* get infos about net device */
#include <net/if.h> /* idem */

/* Software Version */
#define VERSION "<none>"
/* check if privilieges are granted  */
#define IS_ROOT ( ((getuid() != 0) || (geteuid() != 0)) ? 0 : 1 )
/* TODO : this ahs to become an option */
#define MAX_PROBES 1


/*
extern long optind, opterr, optopt;
extern char *optarg;
*/

/** @brief this describes a RARP packet */
typedef struct {
	/** @brief Type of hardware */
	unsigned short us_hwType;
	/** @brief type of protocol */
	unsigned short us_protoType;
	/** @brief length of hardware address in bytes */
	unsigned char  uc_hwLen;
	/** @brief length of the protocol address in bytes */
	unsigned char  uc_protoLen;
	/** @brief Operation code 3 for a request, 4 for a reply // RFC 903 */
	unsigned short us_opcode;
	/** @brief source hardware address */
	char  cht_srcHwAddr[6];
	/** @brief IP address of the source host */
	char  cht_srcIpAddr[4];
	/** @brief hardware address of the remote host */
	char  cht_targetHwAddr[6];
	/** @brief IP address of the remote host */
	char  cht_targetIpAddr[4];
} rarpPacket_t;

/** @brief full ethernet trame, MAC headers + RARP packet as described above */
typedef struct {
	/** @brief Harware address of the device we send to */
	unsigned char  ucht_destHwAddr[6];
	/** @brief Hardware address we of the device we send from */
	unsigned char  ucht_senderHwAddr[6];
	/** @brief ethertype (0x8035 for RARP)*/
	unsigned short us_ethType;
	/** @brief RARP packet
	 * @see rarpPacket_t
	 */
	rarpPacket_t  str_packet;
} etherPacket_t;


/** @brief Contains options/informations given trought command line */
typedef struct {
	/** @brief choosen Interface */
	char * pch_iface;
	/** @brief Subject of the request(s) */
	char * pch_askedHwAddr;
} opt_t;


/* ******************************************************************************************************************************* */

/**
 * @brief Usage() prints out some help to craft a correct command line
 */
void usage ( void );

/**
 * @brief Parse given arguments and fill an opt structure
 *
 * @see opt
 */
signed char argumentManagement ( long l_argc, char **ppch_argv, opt_t *pst_argsDest );

/**
 * @brief perform RARP requests the way defined by user
 */
signed char performRequests ( const opt_t *pst_argsDest );

/**
 * @brief find out and packs the datas we'll send
 */
signed char craftPacket ( etherPacket_t * pstr_packet, char * pch_ifaceName, long l_socket );


/* ******************************************************************************************************************************* */
/* ******************************************************************************************************************************* */


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

	c_retValue = 1;
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
		if (craftPacket(&str_packet, pst_argsDest->pch_iface, l_socket))
			while (++nbProbes < MAX_PROBES);
/*				if (send_probe(l_socket, &str_packet) != 1)
				{
					fprintf(stderr, "Can't send request #%d\n", nbProbes);
				}
				else
				{
#ifdef DEBUG
					fprintf(stderr, "Request #%d sent\n", nbProbes);
#endif
				}*/
		else
		{
			fprintf(stderr, "Can't craft packets\n");
			exit(EXIT_FAILURE);
		}
	
		close(l_socket);
	}
	
	return c_retValue;
}


signed char craftPacket ( etherPacket_t * pstr_packet, char * pch_ifaceName, long l_socket )
{
	signed char c_retValue;
	struct ifreq str_device; /* described in man (7) netdevice */

	c_retValue = 0;

	strncpy(str_device.ifr_name, pch_ifaceName, IFNAMSIZ); /* copy iface name into the ifreq structure we've just declared // IFNAMSIZ defined in net/if.h */

	/* Get local hardware address */
	if (ioctl(l_socket, SIOCGIFHWADDR, &str_device) == -1 )
	{
		perror("ioctl");
		c_retValue = -1;
	}
	else
	{
#ifdef DEBUG
#define MAC(i) (str_device.ifr_hwaddr.sa_data[(i)])
		fprintf(stderr, "Local MAC address is %2x:%2x:%2x:%2x:%2x:%2x\n", MAC(0), MAC(1), MAC(2), MAC(3), MAC(4), MAC(5));
#endif
	}

	return c_retValue;
}

