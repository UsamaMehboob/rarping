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
 * Don't panic reading this code, style is as strange as the coding standarts
 * of the project I'm working on are strict! ;)
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
#include <stdlib.h>/* malloc() */
#include <unistd.h> /* getopt() : command line parsing */
#include <errno.h>
#include <sys/socket.h> /* Network */
#include <sys/types.h> /* for old systems */
#include <arpa/inet.h> /* htons() ... */ 
#include <linux/if_ether.h> /* ETH_P_ALL */
#include <ioctl.h> /* get infos about net device */
#include <if.h> /* idem */

/* Software Version */
#define VERSION "<none>"
/* check if privilieges are granted  */
#define IS_ROOT ( ((getuid() != 0) || (geteuid() != 0)) ? 0 : 1 )
/* TODO : this ahs to become an option */
#define MAX_PROBES 1

/* --- -- --- -- --- -- --- -- --- -- --- -- --- -- --- */


extern int optind, opterr, optopt;
extern char *optarg;

/*
 * RARP packet
 *
 * more : http://www.networksorcery.com/enp/protocol/rarp.htm
 */
typedef struct {
	unsigned short us_hw_type;
	unsigned short us_proto_type;
	unsigned char  uch_hw_len;
	unsigned char  uch_proto_len;
	unsigned short us_opcode;
	unsigned char  ucha_src_mac[6];
	unsigned char  ucha_src_ip[4];
	unsigned char  ucha_target_mac[6];
	unsigned char  ucha_target_ip[4];
} rarp_packet_t;

/*
 * Full Ethernet Trame (IPV4 RARP packet)
 */
typedef struct {
	unsigned char  ucha_dest_mac[6];
	unsigned char  ucha_sender_mac[6];
	unsigned short us_eth_type;
	rarp_packet_t  str_packet;
} ether_packet_t;


/***********************************************************************************************************************************/

/* Contains options/informations given trought command line */
typedef struct {
	char * pch_iface; /* Interface to use to send probes */
	char * pch_dest_mac_addr; /* destination MAC address to craft packet */
} opt_t;

/* Ethernet header */

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
signed char argument_management ( long l_argc, char **ppch_argv, opt_t *pst_args_dest );

/**
 * @brief perform RARP requests the way defined by user
 */
signed char perform_requests ( const opt_t *pst_args_dest );

/* ******************************************************************************************************************************* */
/* ******************************************************************************************************************************* */


int main ( int l_argc, char **ppch_argv )
{
    /* Return value */
    signed char ch_ret_value;
	opt_t str_args;

    ch_ret_value = 0;

    /* Parse args using getopt to fill a struct (args), return < 0 if problem encountered */
    if ( argument_management(l_argc, ppch_argv, &str_args) > 0 )
    {
        /* perform RARP requests as wanted by user (using args struct) */
        ch_ret_value = perform_requests(&str_args);
    }
    else
    {
        /* If any problem occured, explain the user how to build his command line */
        usage();
        /* and exit < 0 */
        ch_ret_value = -1;
    }


    /* Simple exit point */
    return ch_ret_value;
}


signed char argument_management ( long l_argc, char **ppch_argv, opt_t *pst_args_dest )
{
	char ch_ret_value;
	char ch_opt;

	/* Initialisation */
	ch_ret_value = 1;
	pst_args_dest->pch_iface = NULL;
	pst_args_dest->pch_dest_mac_addr = NULL;
	/* ************** */


	/* Parsing options args */
	while ( ( ch_opt = getopt( l_argc, ppc_argv, "I:" ) ) != -1 ) 
	{
		switch(ch_opt)
		{
			case 'I'	:	pst_args_dest->pc_iface = optarg;
							break;

			case 'h'	:
			case '?'	:
			default		:	ch_ret_value = -1;
		}
	}
	
	/* parsing non options args */
	/* The only one must be the MAC Addr we'll request related IP */
	if (optind < l_argc)
		pst_args_dest->pc_dest_mac_addr = ppch_argv[optind];
	else
		c_ret_value = -1;

	/* Check if required infos had been given */
	if ( (pst_args_dest->pc_iface == NULL) || (pst_args_dest->pc_dest_mac_addr == NULL) )
		c_ret_value = -1;
	else
	{
		/* nothing to do */
	}

#ifdef DEBUG
	fprintf(stderr, "Iface : %s\n", pst_args_dest->pc_iface);
	fprintf(stderr, "Iface : %s\n", pst_args_dest->pc_dest_mac_addr);
#endif

	return c_ret_value;
}


void usage ( void )
{
	printf("\n-=] Rarp requests sender [=-\n");
	printf("           ****          \n");
	printf("\nhttp://hatch-the-hitch.blogspot.com\n");
	printf("\nVersion : %s\n", VERSION);
	printf("Usage : ./rarping -I [interface] [request MAC address]\n");
	printf("For example : ./rarping -I eth0 00:03:13:37:be:ef\n\n");

	return;
}


signed char perform_requests ( const opt_t *pst_args_dest )
{
	long l_socket, nb_probes;
	char ch_ret_value;
	ether_packet_t str_packet;

	ch_ret_value = 1;
	nb_probes = 0;

	if ((l_socket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) /* socket opening */
	{
		perror("socket");
		return -1;
	}
	else
	{
		/* nothing to do */
	}

	if (craft_packet(&str_packet, pst_args_dest->pch_iface))
		while (++nb_probes < MAX_PROBES)
			if (send_probe(l_socket, &str_packet) != 1)
			{
				fprintf(stderr, "Can't send request #%d\n", nb_probe);
			}
			else
			{
#ifdef DEBUG
				fprintf(stdout, "Request #%d sent\n", nb_probe);
#endif
			}
	else
	{
		fprintf(stderr, "Ouuch!!\nWill now abort!\n");
		exit(EXIT_FAILURE);
	}
	
	close(l_socket);
	
	return ch_ret_value;
}


signed char craft_packet ( ether_packet_t * pstr_packet, char * pch_iface_name, long l_socket )
{
	struct ifreq str_device; /* described in man (7) netdevice */

	strncpy(str_device.ifr_name, pch_iface_name, IFNAMESIZ);

	if (ioctl(l_socket, SIOCGIFHWADDR, &str_device) == -1 )
	{
		perror("ioctl");
		exit(EXIT_FAILURE);
	}
	else
	{
		/* nothing to do */
	}
}

