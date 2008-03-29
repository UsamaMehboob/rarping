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
	unsigned short hw_type;
	unsigned short proto_type;
	unsigned char  hw_len;
	unsigned char  proto_len;
	unsigned short opcode;
	unsigned char  src_mac[6];
	unsigned char  src_ip[4];
	unsigned char  target_mac[6];
	unsigned char  target_ip[4];
} rarp_packet_t;

/*
 * Full Ethernet Trame (IPV4 RARP packet)
 */
typedef struct {
	unsigned char  dest_mac[6];
	unsigned char  sender_mac[6];
	unsigned short eth_type;
	rarp_packet_t  str_packet;
} ether_packet_t;


/***********************************************************************************************************************************/

/* Contains options/informations given trought command line */
typedef struct {
	char * pc_iface; /* Interface to use to send probes */
	char * pc_dest_mac_addr; /* destination MAC address to craft packet */
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
signed char argument_management ( int i_argc, char **ppc_argv, opt_t *pst_args_dest );

/**
 * @brief perform RARP requests the way defined by user
 */
signed char perform_requests ( const opt_t *pst_args_dest );

/* ******************************************************************************************************************************* */
/* ******************************************************************************************************************************* */


int main ( int argc, char **argv )
{
    /* Return value */
    unsigned char uc_ret;
	opt_t args;

    uc_ret = 0;

    /* Parse args using getopt to fill a struct (args), return < 0 if problem encountered */
    if ( argument_management(argc, argv, &args) > 0 )
    {
        /* perform RARP requests as wanted by user (using args struct) */
        uc_ret = perform_requests(&args);
    }
    else
    {
        /* If any problem occured, explain the user how to build his command line */
        usage();
        /* and exit < 0 */
        uc_ret = -1;
    }


    /* Simple exit point */
    return uc_ret;
}


signed char argument_management ( int i_argc, char **ppc_argv, opt_t *pst_args_dest )
{
	char c_ret_value;
	char c_opt;

	/* Initialisation */
	c_ret_value = 1;
	pst_args_dest->pc_iface = NULL;
	pst_args_dest->pc_dest_mac_addr = NULL;
	/* ************** */


	/* Parsing options args */
	while ( ( c_opt = getopt( i_argc, ppc_argv, "I:" ) ) != -1 ) 
	{
		switch(c_opt)
		{
			case 'I'	:	pst_args_dest->pc_iface = optarg;
							break;

			case 'h'	:
			case '?'	:
			default		:	c_ret_value = -1;
		}
	}
	
	/* parsing non options args */
	/* The only one must be the MAC Addr we'll request related IP */
	if (optind < i_argc)
		pst_args_dest->pc_dest_mac_addr = ppc_argv[optind];
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

#endif 0
	if (craft_packet(&str_packet))
		while (++nb_probes < MAX_PROBES)
			if (send_probe(l_socket, &str_packet) != 1)
				continue; /* TODO : alert here */
			else
			{
				/* nothing to do */
			}
	else
	{
		fprintf(stderr, "Ouuch!!\nWill now abort!\n");
		exit(EXIT_FAILURE);
	}
#endif
	close(l_socket);
	
	return ch_ret_value;
}


