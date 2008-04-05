/**
 * @file rarping.h
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

#ifndef RARPING_H
#define RARPING_H

/*
 * Standard includes
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
#include <linux/if_packet.h> /* struct sockaddr_ll */
#include <arpa/inet.h> /* htons() ... */

/* Software Version */
#define VERSION "<none>"
/* check if privilieges are granted  */
#define IS_ROOT ( ((getuid() != 0) || (geteuid() != 0)) ? 0 : 1 )
/* TODO : this ahs to become an option */
#define MAX_PROBES 1
/* ethertype for the rarp protocol */
#ifndef ETH_TYPE_RARP
#define ETH_TYPE_RARP 0x8035
#endif



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
	unsigned char  cht_srcHwAddr[6];
	/** @brief IP address of the source host */
	unsigned char  cht_srcIpAddr[4];
	/** @brief hardware address of the remote host */
	unsigned char  cht_targetHwAddr[6];
	/** @brief IP address of the remote host */
	unsigned char  cht_targetIpAddr[4];
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
	unsigned char * pch_iface;
	/** @brief Subject of the request(s) */
	unsigned char * pch_askedHwAddr;
} opt_t;


/* --- -- --- Functions prototypes --- -- --- */
/**
 * @brief Usage() prints out some help to craft a correct command line
 */
void usage ( void );

/**
 * @brief Parse given arguments and fill an opt structure
 *
 * @see opt_t
 */
signed char argumentManagement ( long l_argc, char **ppch_argv, opt_t *pst_argsDest );

/**
 * @brief perform RARP requests the way defined by user
 */
signed char performRequests ( const opt_t *pst_argsDest );

/**
 * @brief find out and packs the datas we'll send
 */
signed char craftPacket ( etherPacket_t * pstr_packet, unsigned char * pch_ifaceName, struct sockaddr_ll * pstr_device, const unsigned char * pch_askedHwAddr, long l_socket );

/**
 * @brief fills a sockaddr_ll struct whith informations on low level access
 */
char getLowLevelInfos ( struct sockaddr_ll * pstr_device, unsigned char * pch_ifaceName, long l_socket );

/**
 * @brief fill field hw_addr of pstr_device with local MAC address
 */
char getLocalHardwareAddress ( long l_socket, unsigned char * pch_ifaceName, unsigned char * mac );

/**
 * @brief get Index of used network interface
 */
unsigned long getIfaceIndex ( unsigned char * pch_ifName, long l_socket );

/**
 * @brief send RARP requests into the wild
 */
signed char sendProbe ( long l_socket, etherPacket_t * pstr_packet, struct sockaddr_ll * pstr_device );
/* --- -- --- -- --- -- --- -- --- -- --- -- --- */


#endif /* RARPING_H */


