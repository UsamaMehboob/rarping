/**
 * @file rarping.h
 *
 * @brief Rarping - send RARP REQUEST to a neighbour host
 *
 * $Author$
 * $Date$
 *
 * $Revision$
 */

/* 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
#include <getopt.h> /* getopt() long options parsing */
#include <sys/time.h> /* struc timeval : used to set timeout*/
#include <signal.h> /* signal() */
#include <errno.h>
#include <sys/socket.h> /* Network */
#include <netinet/in.h> /* inet_ntoa */
#include <sys/types.h> /* for old systems */
#include <arpa/inet.h> /* htons() ... */ 
#include <linux/if_ether.h> /* ETH_P_ALL */
#include <sys/ioctl.h> /* get infos about net device */
#include <net/if.h> /* idem */
#include <linux/if_packet.h> /* struct sockaddr_ll */
#include <arpa/inet.h> /* htons() ... */

/*
 * Defines
 */
/** @def VERSION
 * Software Version */
#define VERSION "rarping 0.1 beta"

#define BOOL char
#define FALSE 0
#define TRUE (!FALSE)

/*
 * Macros
 */

/** @def IS_ROOT
 * check if privilieges are granted  */
#define IS_ROOT ( ((getuid() != 0) || (geteuid() != 0)) ? 0 : 1 )

/** @def ABS(i)
 * send positive part of (i) */
#define ABS(i) (((i) >= 0) ? (i) : -(i))

/*
 * Const
 */

/** @def ERR_ARG_PARSING
 * returned value if argument parsing fails */
#define ERR_ARG_PARSING -1

/* ethertype for the rarp protocol */
#ifndef ETH_TYPE_RARP
/** @def ETH_TYPE_RARP
 * ether type for the rarp protocol */
#define ETH_TYPE_RARP 0x8035
#endif

/** @def IP_PROTO
 * IP protocole number */
#define IP_PROTO 0x800
/** @def HW_TYPE_ETHERNET
 * hardware type (ethernet) */
#define HW_TYPE_ETHERNET 0x01
/** @def RARP_OPCODE_REQUEST
 * operation code for a RARP request */
#define RARP_OPCODE_REQUEST 0x03
/** @def RARP_OPCODE_REPLY
 * operation code for a RARP reply */
#define RARP_OPCODE_REPLY 0x0004

/** @def IP_ADDR_SIZE
 * max length of an IP address in standard notation */
#define IP_ADDR_SIZE 15
/** @def MAC_ADDR_SIZE
 * max length of a MAC address in standard notation */
#define MAC_ADDR_SIZE 17

/** @def S_TIMEOUT
 * number of seconds to spend when trying to send/recv */
#define S_TIMEOUT_DEFAULT 1
/** @def US_TIMEOUT
 * number of microseconds to spend when trying to send/recv */
#define US_TIMEOUT_DEFAULT 0
/** @def MS_DEFAULT_DELAY
 * number of milliseconds to sleep between two probes */
#define MS_DEFAULT_DELAY 1000



/*
 * Typedef
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
    unsigned char  uct_srcHwAddr[6];
    /** @brief IP address of the source host */
    unsigned char  uct_srcIpAddr[4];
    /** @brief hardware address of the remote host */
    unsigned char  uct_targetHwAddr[6];
    /** @brief IP address of the remote host */
    unsigned char  uct_targetIpAddr[4];
} rarpPacket_t;


/** @brief full ethernet trame, MAC headers + RARP packet as described above */
typedef struct {
    /** @brief Harware address of the device datas are send to */
    unsigned char  uct_destHwAddr[6];
    /** @brief Hardware address of the device datas are send from */
    unsigned char  uct_senderHwAddr[6];
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
    /** @brief if RARP replies are sent, this is the answer */
    char * pch_IpAddrRarpReplies;
    /** @brief IP address to use */
    char * pch_spoofedLocalIpAddress;
    /** @brief Number of requests to send */
    unsigned long ul_count;
    /** @brief Perform an infinite number of retries or not, boolean */
    unsigned char uc_unlimitedRetries;
    /** @brief if limited number of retries, this is specified here */
    unsigned long ul_maximumRetries;
    /** @brief as name suggests : exit after a reply */
    unsigned char uc_exitOnReply;
    /** @brief type of packets to send (requests or replies) */
    unsigned char uc_choosenOpCode;
    /** @brief timeout on send/recv */
    struct timeval str_timeout;
    /** @brief number of milliseconds to wait between two probes */
    unsigned long ul_waitingMilliSeconds; 
} opt_t;


/* --- -- --- Functions prototypes --- -- --- */
/**
 * @brief Usage() prints out some help to craft a correct command line
 */
void usage ( void );


/**
 * @brief set the send/recv timeout from command line to integers
 * @param pstr_timeout vdestination of parsed argument
 * @param pch_arg timeout, as string, given by user through command line
 * @return none
 */
void parseTimeout ( struct timeval * pstr_timeout, char * pch_arg );


/**
 * @brief Parse given arguments and fill an opt structure
 * @param l_argc is the number of arguments contained in command line when launching the soft
 * @param ppch_argv is an array of pointers to the arguments given
 * @param pst_argsDest is a structure where are stored user defined options
 * @return Error code according to the execution of the function
 * @retval 0 The function ends normally
 * @retval ERR_ARG_PARSING (< 0) can't parse command line
 * @see opt_t
 */
signed char argumentManagement ( long l_argc, char **ppch_argv, opt_t *pstr_argsDest );


/**
 * @brief set options to their default values
 * @param pstr_argsDest points to the structure where user choices are stored
 * @see opt_t
 */ 
void initOptionsDefault ( opt_t * pstr_args );


/**
 * @brief turn on the signal handler
 */
void signalHandler ( void );


/**
 * @brief perform RARP requests the way defined by user
 * @param pst_argsDest is an opt_t structure which contains user defined options
 * @return Error code according to the execution of the function
 * @retval 0 The function ends normally
 * @retval -1 can't open RAW socket
 * @retval -3 can't craft packet
 * @retval -4 error when closing socket
 */
signed char performRequests ( const opt_t *pstr_argsDest );


/**
 * @brief find out and packs the datas we'll send
 * @param pstr_packet points to a struct this function will fill with raw datas to send
 * @param pch_ifaceName is a string containing the interface to use
 * @param pstr_device points to a structure that will contain low level informations needed
 * @param pch_askedHwAddr (string) contains the hardware address to request about
 * @param l_socket is the raw socket opened before
 * @return Error code according to the execution of the function
 * @retval 0 the function ends normally
 * @retval -1 bad interface given
 * @retval -2 bad hardware address given, or unrecognized format
 */
signed char craftPacket ( etherPacket_t * pstr_packet, const opt_t * str_destArgs, struct sockaddr_ll * pstr_device, long l_socket );


/**
 * @brief fills a sockaddr_ll struct whith hardware address and interface index of a selected device
 * @param pstr_device contains provides a low level device independant access
 * @param pch_ifaceName (string) contains the hardware address to request about
 * @param l_socket raw socket opened before
 * @return Error code according to the execution of the function
 * @retval 0 the function ends normally
 * @retval -1 if function can't provide low level infos
 */
signed char getLowLevelInfos ( struct sockaddr_ll * pstr_device, char * pch_ifaceName, long l_socket );


/**
 * @brief fill hw_addr field of pstr_device with local MAC address
 * @param l_socket raw socket opened befor
 * @param pch_ifaceName (string) contains the name of the selected interface as provided by user
 * @param mac string filled with the hardware address of the selected interface
 * @return Error code according to the execution of the function
 * @retval 0 the function ends normally
 * @retval -1 can't find MAC address
 */
signed char getLocalHardwareAddress ( long l_socket, char * pch_ifaceName, unsigned char * puc_mac );


/**
 * @brief get Index of used network interface
 * @param pch_ifName (string) contains the name of the selected interface as provided by user
 * @param l_socket raw socket opened befor
 * @return interface index
 */
unsigned long getIfaceIndex ( char * pch_ifName, long l_socket );


/**
 * @brief send RARP requests into the wild
 * @param l_socket raw socket opened before
 * @param pstr_packet raw datas to send
 * @param pstr_device low level informations to send datas without any encapsulation
 * @return Error code according to the execution of the function
 * @retval 0 the function ends normally
 * @return -1 can't perform sending
 */
signed char sendProbe ( long l_socket, etherPacket_t * pstr_packet, struct sockaddr_ll * pstr_device );


/**
 * @brief wait for an answer and, if receive any, parse it
 * @param l_socket is the raw socket opened before
 * @param pstr_device low level informations to receive datas without any encapsulation
 * @param str_sendingMoment is used to chronometer and print out how fast the received replies are
 * @return the number of replies received
 */
unsigned char getAnswer ( long l_socket, struct sockaddr_ll * pstr_device, const struct timeval str_sendingMoment );


/**
 * @brief check if received packet seems correct (or not) it and print its main informations
 * @param pstr_reply points to the received packet
 * @param str_delay is the time elapsed between sending and reeption
 */
void printOutReply ( etherPacket_t * pstr_reply, const struct timeval str_delay );


/**
 * @brief parse an reply packet and fill strings to print results out in a clean way
 * @param pstr_reply datas to parse (received packet)
 * @param tch_replySrcIp formatted string containing IP address of the sender of the reply
 * @param tch_replySrcHwAddr formatted string containing MAC address of the sender of the reply
 * @param tch_replyHwAddr formatted string containing MAC address we were requesting
 * @param tch_replyAddrIp formatted string containing IP address we were asking for
 * @return Error code according to the execution of the function
 * @retval 0 the function ends normally
 */
signed char parse ( etherPacket_t * pstr_reply, char tch_replySrcIp[], char tch_replySrcHwAddr[], char tch_replyHwAddr[], char tch_replyAddrIp[] );


/**
 * @brief print out a summary of what happened
 * @param ul_sentPackets number of sent packets
 * @param ul_receivedPackets number of received packets
 * @return  Error code according to the execution of the function
 * @retval 0 the function ends normally
 * @retval -1 not any packet were sent
 */
signed char footer ( unsigned long ul_sentPackets, unsigned long ul_receivedPackets );


/**
 * @brief open a raw socket and set timeout on sending and reception
 * @param none
 * @return return code of socket() : > is the socket we want, < 0 if an error ocured
 */
signed long openRawSocket ( struct timeval str_timeout );


/**
 * @brief sending loop : send packets as many times as wanted
 * @param pstr_argsDest points to the structure which contains user inputs
 * @param pstr_packet points to the packet to send
 * @param sockaddr_ll low level information to send datas without encapsulation
 * @param l_socket Raw socket opened before
 * @return Error code according to the execution of the function
 * @retval 0 the function ends normally
 */
signed char loop( const opt_t * pstr_argsDest, etherPacket_t * pstr_packet, struct sockaddr_ll * pstr_device, long l_socket );


/**
 * @brief fills target protocol address in sent packets according to user choices (option -a)
 * @param puc_targetIpAddress target protocol address field in crafted packet
 * @param pstr_argsDest choosen user's inputs
 * @return Error code according to the execution of the function
 * @retval 0 the function ends normally
 */
signed char setTargetIpAddress ( unsigned char * puc_targetIpAddress, const opt_t * pstr_argsDest );


/**
 * @brief set source IP address in packets to wanted values
 * @param puc_senderIpAddress field to fill
 * @param pstr_argsDest user options
 * @param l_socket socket descriptor to perform ioctl calls (to find ut local real IP address)
 * @retval error code according to the execution of the function
 * @return 0 the function ends normally
 * @return 1 an error occured : IP address set to its default value : 0.0.0.0
 */
signed char setSenderIpAddress ( unsigned char * puc_senderIpAddress, const opt_t * pstr_argsDest, long l_socket );


/**
 * @brief return elapsed time between two given moments
 * @param str_beginning starting point
 * @param str_termination ending point
 * @return struct timeval containing elapsed time from str_beginning to str_termination
 */
struct timeval timeDiff ( const struct timeval str_beginning, const struct timeval str_termination );


/**
 * @brief print out elapsed time from a timeval structure to usual format in milliseconds
 * @param str_time contains a number of seconds and microseconds
 */
void printTime_ms ( const struct timeval str_time );


/**
 * @brief perform a clean exit, calling statistics function and closing the socket
 * @param int sig received signal that force rarping to exit
 */
void rarpingOnExit ( int sig );
/* --- -- --- -- --- -- --- -- --- -- --- -- --- */


#endif /* RARPING_H */

