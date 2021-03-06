/**
 * @file rarping.c
 *
 * @brief Rarping - send RARP REQUEST to a neighbour host
 * @see RFC 903
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


#include "rarping.h"

/* Global copy of the raw socket file descriptor : used to call close() on sigterm */
long l_SockRaw = 0;
/* -------- */
/* Global counters : current probe number and total number of received replies */
unsigned long ul_NbProbes, ul_ReceivedReplies;
/* -------- */



int main( int i_argc, char **ppch_argv )
{
    /* Return value */
    signed char c_retValue;
    long l_argc;
    opt_t str_args;

    l_argc     = i_argc; /* if sizeof(int) != sizeof(long) */
    c_retValue = 0;

    /* Parse args using getopt to fill a struct (args), return < 0 if problem encountered */
    if ( argumentManagement( l_argc, ppch_argv, &str_args ) > 0 )
    {
        /* We turn on the signal handler */
        signalHandler();
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


signed char argumentManagement( long l_argc, char **ppch_argv, opt_t *pstr_argsDest )
{
    signed char c_retValue;
    long l_opt, l_optIndex;
    static struct option tstr_longOpt[] = {
        { "interface",    required_argument, 0, 'I' },
        { "count",        required_argument, 0, 'c' },
        { "send-replies", required_argument, 0, 'a' },
        { "timeout",      required_argument, 0, 't' },
        { "delay",        required_argument, 0, 'w' },
        { "retries",      required_argument, 0, 'r' },
        { "source-ip",    required_argument, 0, 's' },
        { "exit-on-reply",      no_argument, 0, 'q' },
        { "version",            no_argument, 0, 'V' },
        { "help",               no_argument, 0, 'h' },
        { 0, 0, 0, 0 }
    };

    /* Initialisation */
    c_retValue = 1;
    l_optIndex = 0;
    initOptionsDefault( pstr_argsDest );
    /* ************** */
    /* External variables init */
    optarg = NULL;
    optind = 1;
    opterr = 1; /* Set to zero to inhibit error messages on unrecognized options */
    optopt = '?'; /* Char to return on unrecognized option */


    /* Parsing options args */
    while ( ( l_opt = getopt_long( l_argc, ppch_argv, "I:c:t:a:w:r:s:qVh", tstr_longOpt, ( int * )&l_optIndex ) ) != -1 ) 
    {
        switch( l_opt )
        {
            /* Interface to use */
            case 'I'    :   pstr_argsDest->pch_iface = optarg;
                            break;

            /* number of packets to send (infinite if nothing specified */
            case 'c'    :   pstr_argsDest->ul_count = ABS( atol( optarg ) ); /* < 0 were stupid */
                            if ( pstr_argsDest->ul_count == 0 )
                                c_retValue = ERR_ARG_PARSING;
                            break;

            /* Send RARP replies instead of requests */
            case 'a'    :   pstr_argsDest->uc_choosenOpCode = RARP_OPCODE_REPLY;
                            pstr_argsDest->pch_IpAddrRarpReplies = optarg;/* content of the reply (IP address) */
                            break;

            /* set timeout */
            case 't'    :   parseTimeout( &( pstr_argsDest->str_timeout ), optarg );
                            break;
			
            /* pause between probes */
            case 'w'    :   pstr_argsDest->ul_waitingMilliSeconds = 0;
                            pstr_argsDest->ul_waitingMilliSeconds = ABS( atol( optarg ) );
                            if ( pstr_argsDest->ul_waitingMilliSeconds == 0 )
                            {
                                fprintf( stderr, "Ivalid delay (%s), must be an integer, of milliseconds\n", optarg );
                                exit( EXIT_FAILURE );
                            }
                            break;

            /* retries on unanswered probes */
            case 'r'    :   pstr_argsDest->uc_unlimitedRetries = 0;
                            pstr_argsDest->ul_maximumRetries = ABS( atol( optarg ) ); /* if incorrect => atol send us zero, this will be good too */
                            break;

            /* spoof local IP address */                
            case 's'    :   pstr_argsDest->pch_spoofedLocalIpAddress = optarg;
                            break;

            /* exit on first catched reply */
            case 'q'    :   pstr_argsDest->uc_exitOnReply = 1;
                            break;

            /* print version and exit */
            case 'V'    :   fprintf( stdout, "%s\n", VERSION );
                            exit( EXIT_FAILURE );
                            break;

            /* print out a short help mesage */
            case 'h'    :
            case '?'    :
            default     :   c_retValue = ERR_ARG_PARSING;
        }
    }
	
    /* parsing non options args */
    /* The only one must be the MAC Addr we'll request related IP */
    if ( optind < l_argc )
        pstr_argsDest->pch_askedHwAddr = ppch_argv[optind];
    else
        c_retValue = ERR_ARG_PARSING;

    /* Check if required infos had been given */
    if ( ( pstr_argsDest->pch_iface == NULL ) || ( pstr_argsDest->pch_askedHwAddr == NULL ) )
        c_retValue = ERR_ARG_PARSING;
    else
    {
        fprintf( stdout, "RARPING %s on %s\n", pstr_argsDest->pch_askedHwAddr, pstr_argsDest->pch_iface );
    }

    return c_retValue;
}


void initOptionsDefault( opt_t * pstr_args )
{
    pstr_args->pch_iface                 = NULL;
    pstr_args->pch_askedHwAddr           = NULL;
    pstr_args->pch_IpAddrRarpReplies     = NULL;
    pstr_args->pch_spoofedLocalIpAddress = NULL;
    pstr_args->ul_count                  = 0;
    pstr_args->uc_unlimitedRetries       = 1; /* Default behavior is to perform an infinite number of retries */
    pstr_args->ul_maximumRetries         = 0;
    pstr_args->uc_exitOnReply            = 0;
    pstr_args->uc_choosenOpCode          = RARP_OPCODE_REQUEST;
    pstr_args->str_timeout.tv_sec        = S_TIMEOUT_DEFAULT;
    pstr_args->str_timeout.tv_usec       = US_TIMEOUT_DEFAULT;
    pstr_args->ul_waitingMilliSeconds    = MS_DEFAULT_DELAY;	
    return;
}

void usage( void )
{
    /*
     * Usage function : splitted option per option for readability issues
     */
    fprintf( stderr, "Usage : ./rarping [options] [-I interface] request_MAC_address\n" );

    fprintf( stderr, "-h, --help\n"
                    "\tprint this screen and exit\n" );

    fprintf( stderr, "-V, --version\n"
                    "\tprint version and exit\n" );

    fprintf( stderr, "-q, --exit-on-reply\n"
                    "\tExit after receiving a reply\n" );

    fprintf( stderr, "-c, --count [count]\n"
                    "\tsend [count] request(s) and exit\n" );

    fprintf( stderr, "-a, --send-replies [IP address]\n"
                    "\tsend replies instead of requests, [IP address] is the content of the reply\n" );

    fprintf( stderr, "-s, --source-ip [IP address]\n"
                    "\tuse [IP address] in sent probes instead of real one\n" );

    fprintf( stderr, "-t, --timeout [timeout]\n"
                    "\tset the send/recv timeout value to [timeout] milliseconds (default 1000)\n" );

    fprintf( stderr, "-w, --delay [delay]\n"
                    "\tset the delay between two probes to [delay] milliseconds (default 1000)\n" );

    fprintf( stderr, "-r, --retries [retries]\n"
                    "\tabort after [retries] unanswered probes (default none)\n" );

    fprintf( stderr, "-I, --interface [interface]\n"
                    "\tnetwork device to use (REQUIRED)\n" );

    fprintf( stderr, "request_MAC_address : hardware address we request associated IP address (REQUIRED)\n" );
    fprintf( stderr, "For example : ./rarping -I eth0 00:03:13:37:be:ef\n" );
}


void parseTimeout( struct timeval * pstr_timeout, char * pch_arg )
{
    unsigned long ul_tmpTimeout;

    if ( sscanf( pch_arg, "%lu", &ul_tmpTimeout ) != 1 )
    {
        fprintf( stderr, "Unrecognized format %s for a timeout : must be a positive ineger (in milliseconds)\n", pch_arg );
        fprintf( stderr, "using default : 1000 milliseconds\n" );
        pstr_timeout->tv_sec  = S_TIMEOUT_DEFAULT;
        pstr_timeout->tv_usec = US_TIMEOUT_DEFAULT;
    }
    else
    {
        /* User specified timeout is in milliseconds and the timeval struct deals with seconds and micro-seconds */
        pstr_timeout->tv_sec  = ul_tmpTimeout / 1000; /* destructive division */
        pstr_timeout->tv_usec = ( ul_tmpTimeout % 1000 ) * ( long )1000;
    }
}


void signalHandler( void )
{
    /* SIGINT is sent on ctrl+c */
    signal( SIGINT, rarpingOnExit );
}


signed char performRequests( const opt_t *pstr_argsDest )
{
    long l_socket;
    signed char c_retValue;
    etherPacket_t str_packet;
    struct sockaddr_ll str_device; /* device independant physical layer address */

    c_retValue = 0;
    /* These two ones are globals :( */
    ul_NbProbes = 0;
    ul_ReceivedReplies = 0;
    /* --- */
    
    banner(); /* print out an awesome starting session banner */

    if ( ( l_socket = openRawSocket( pstr_argsDest->str_timeout ) ) < 0 )
    {
        c_retValue = -1;
    }
    else
    {
        /* if packet can be correctly crafted... */
        if ( craftPacket( &str_packet, pstr_argsDest, &str_device, l_socket ) == 0 )
        {
            /* Sending/Receiving Loop */
            loop( pstr_argsDest, &str_packet, &str_device, l_socket );
        }
        else
        {
            fprintf( stderr, "Can't craft packets\n" );
            c_retValue = -3;
        }
        /* This code will be executed whatever happens, except catching signals, after socket has been (correctly) opened */
        footer( ul_NbProbes, ul_ReceivedReplies );
		
        shutdown( l_socket, SHUT_RDWR ); /* as would do every gentleman!! */
        if ( close( l_socket ) < 0 )
        {
            perror( "close" );
            c_retValue = -4;
        }
    }

    /* This code will be executed every time */
    return c_retValue;
}

void banner( void )
{
    char buff[26] = "";
    time_t now;

    time( &now );
    ctime_r( &now, buff );
    fprintf( stdout, "\n--] Starting RARPing session at %s\n", buff );
}

signed char craftPacket( etherPacket_t * pstr_packet, const opt_t * pstr_destArgs, struct sockaddr_ll * pstr_device, long l_socket )
{
    signed char c_retValue;

    c_retValue = 0;


    if ( getLowLevelInfos( pstr_device, pstr_destArgs->pch_iface, l_socket ) < 0 )
    {
        fprintf( stderr, "Critical : can't access device level on %s\n", pstr_destArgs->pch_iface );
        c_retValue = -1;
    }
    else
    {
        /* Craft Packet */
        memset( pstr_packet->tuc_destHwAddr, 0xFF, 6 );
        memcpy( pstr_packet->tuc_senderHwAddr, pstr_device->sll_addr, 6 );
        pstr_packet->us_ethType = htons( ETH_TYPE_RARP );
        pstr_packet->str_packet.us_hwType = htons( HW_TYPE_ETHERNET );
        pstr_packet->str_packet.us_protoType = htons( IP_PROTO );
        pstr_packet->str_packet.uc_hwLen = 6; /* length of mac address in bytes */
        pstr_packet->str_packet.uc_protoLen = 4; /* Were're in IPV4 here */
        pstr_packet->str_packet.us_opcode = htons( pstr_destArgs->uc_choosenOpCode );
        memcpy( pstr_packet->str_packet.tuc_srcHwAddr, pstr_device->sll_addr, 6 );
        /* In a RARP request this field is undefined */
        setSenderIpAddress( pstr_packet->str_packet.tuc_srcIpAddr, pstr_destArgs, l_socket );
        /* set this field to 0 for a request, wanted IP address for a reply */
        setTargetIpAddress( pstr_packet->str_packet.tuc_targetIpAddr, pstr_destArgs );
        /* --- -- --- -- --- -- --- -- --- -- --- -- -- */
#define MAC_FIELD(a) (&(pstr_packet->str_packet.tuc_targetHwAddr[(a)]))
        if ( sscanf( pstr_destArgs->pch_askedHwAddr, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", MAC_FIELD(0), MAC_FIELD(1), MAC_FIELD(2), MAC_FIELD(3), MAC_FIELD(4), MAC_FIELD(5)) != 6 )
        {
            fprintf( stderr, "Unrecognised format %s for a MAC address\n", pstr_destArgs->pch_askedHwAddr );
            fprintf( stderr, "Use : aa:bb:cc:dd:ee:ff notation\n" );
            c_retValue = -2;
        }
        else
        {
            /* nothing to do */
        }
    }

    return c_retValue;
}


signed char getLowLevelInfos( struct sockaddr_ll * pstr_device, char * pch_ifaceName, long l_socket )
{
    signed char c_retValue;

    /* --- Init --- */
    c_retValue = 0;
    memset( pstr_device, 0, sizeof( struct sockaddr_ll ) );
    /* --- -- --- */

    pstr_device->sll_family   = AF_PACKET;
    pstr_device->sll_protocol = htons( ETH_P_ALL );
    pstr_device->sll_ifindex  = getIfaceIndex( pch_ifaceName, l_socket );
    pstr_device->sll_halen    = 6; /* Hardware address length */

    if ( getLocalHardwareAddress( l_socket, pch_ifaceName, pstr_device->sll_addr ) < 0 )
    {
        fprintf( stderr, "Can't find local hardware address (MAC address)\n" );
        c_retValue = -1;
    }
    else
    {
        /* nothing to do */
    }

    return c_retValue;
}


signed char getLocalHardwareAddress( long l_socket, char * pch_ifaceName, unsigned char * puc_mac )
{
    signed char c_retValue;
    struct ifreq str_tmpIfr; /* described in man (7) netdevice */

    c_retValue = 0;
    memset( &str_tmpIfr, 0, sizeof( struct ifreq ) );
    strncpy( str_tmpIfr.ifr_name, pch_ifaceName, IFNAMSIZ-1 );  /* copy iface name into the ifreq structure we've just declared // IFNAMSIZ defined in net/if.h */

    /* Get local hardware address */

    if ( ioctl( l_socket, SIOCGIFHWADDR, &str_tmpIfr ) == -1 )
    {
        perror( "ioctl" );
        c_retValue = -1;
    }
    else
    {
        memcpy( puc_mac, str_tmpIfr.ifr_hwaddr.sa_data, 6 ); /* cpoy the local MAC address into mac buffer (into struct sockaddr_sll str_device in fact)*/
    }

    return c_retValue;
}


unsigned long getIfaceIndex( char * pch_ifName, long l_socket )
{
    struct ifreq str_tmpIfr;

    memset( &str_tmpIfr, 0, sizeof( struct ifreq ) );
    strncpy( str_tmpIfr.ifr_name, pch_ifName, IF_NAMESIZE-1 );

    /* get card Index */
    if( ioctl( l_socket, SIOCGIFINDEX, &str_tmpIfr ) < 0 )
    {
        perror( "ioctl" );
    }
    else
    {
        /* nothing to do */
    }
    
    return str_tmpIfr.ifr_ifindex;
}


signed char sendProbe( long l_socket, etherPacket_t * pstr_packet, struct sockaddr_ll * pstr_device )
{
    signed char c_retValue;

    c_retValue = 1;

    if ( sendto( l_socket, pstr_packet, sizeof( etherPacket_t ), 0, 
                ( const struct sockaddr * )pstr_device, sizeof( struct sockaddr_ll ) ) <= 0 )
    {
        perror( "Sendto" );
        c_retValue = -1;
    }
    return c_retValue;
}


unsigned char getAnswer( long l_socket, struct sockaddr_ll * pstr_device, const struct timeval str_sendingMoment )
{
    etherPacket_t str_reply; /* to store received datas */
    struct sockaddr_ll str_from;
    unsigned char uc_retValue;
    socklen_t addrLen_t; /* Address length */
    signed long l_reception; /* Number of bytes received or error flag */
    struct timeval str_recvMoment, str_delay; /* delay is the time elapsed between sending and reception */

    /* usual initialisation */
    uc_retValue = 1;
    l_reception = 0;
    memset( &str_reply, 0, sizeof( etherPacket_t ) );
    addrLen_t = sizeof( struct sockaddr_in );
    /* --- -- --- -- --- -- --- -- */
	
#ifdef DEBUG
    fprintf( stderr,"Waiting for a reply...\n" );
#endif

    /* Reception */
    switch ( l_reception = recvfrom( l_socket, &str_reply, 
                sizeof( etherPacket_t ), 0, ( struct sockaddr * )&str_from, &addrLen_t ) )
    {
        case    -1    :	if ( errno != EAGAIN ) /* EAGAIN means timeout were reached */
                            perror( "recvfrom" );
                        uc_retValue = 0;

        case 	0     : break;

        default       : /* Time recording */
                        gettimeofday( &str_recvMoment, NULL );
                        str_delay = timeDiff( str_sendingMoment, str_recvMoment );
                        printOutReply( &str_reply, str_delay );
    }

    return uc_retValue;
}


void printOutReply( etherPacket_t * pstr_reply, const struct timeval str_delay )
{
    /* strings to print out results in a clean way */
    char tch_replySrcIp[IP_ADDR_SIZE+1]      = "", 
         tch_replySrcHwAddr[MAC_ADDR_SIZE+1] = "", 
         tch_replyHwAddr[MAC_ADDR_SIZE+1]    = "", 
         tch_replyAddrIp[IP_ADDR_SIZE+1]     = "";
	
    /* If received packet is a RARP reply */
    if ( pstr_reply->us_ethType == htons( ETH_TYPE_RARP ) )
    {
        /* we craft strings to print results using received packet */
        parse( pstr_reply, tch_replySrcIp, tch_replySrcHwAddr, tch_replyHwAddr, tch_replyAddrIp );

#define OPERATION(o) ( ( o == htons(RARP_OPCODE_REPLY) ) ? "Reply" : "Request" )
        fprintf( stdout, "%s received from %s (%s) : %s is at %s ", 
                OPERATION( pstr_reply->str_packet.us_opcode ), 
                tch_replySrcIp, tch_replySrcHwAddr, tch_replyHwAddr, tch_replyAddrIp );
        printTime_ms( str_delay );
        fprintf( stdout, "\n" );
    }
    else
    {
#define __MAC(i) (pstr_reply->tuc_senderHwAddr[(i)])
        fprintf( stdout, "Unknown packet received (ether type = 0x%04x) from %02x:%02x:%02x:%02x:%02x:%02x ", 
                ntohs( pstr_reply->us_ethType ), 
                __MAC(0), __MAC(1), __MAC(2), __MAC(3), __MAC(4), __MAC(5) );
        printTime_ms( str_delay );
        fprintf( stdout, "\n" ); 
    }
}


signed char parse( etherPacket_t * pstr_reply, char tch_replySrcIp[], char tch_replySrcHwAddr[], char tch_replyHwAddr[], char tch_replyAddrIp[] )
{
    struct in_addr str_tmpIpAddr;

    /* Fill the string tch_srcIpAddr, that is the IP address of the reply sender with the address contained in received packet */
    memset( &str_tmpIpAddr, 0, sizeof( struct in_addr ) );
    memcpy( &str_tmpIpAddr, pstr_reply->str_packet.tuc_srcIpAddr, 4 );
    strncpy( tch_replySrcIp, inet_ntoa( str_tmpIpAddr ), IP_ADDR_SIZE );

    /* 
     * Fill the string tch_replyIpAddr, that is the IP address of the host which MAC address is the one we requested about
     * This is the "real" answer to the request sent
     */
    memset( &str_tmpIpAddr, 0, sizeof( struct in_addr ) );
    memcpy( &str_tmpIpAddr, pstr_reply->str_packet.tuc_targetIpAddr, 4 );
    strncpy( tch_replyAddrIp, inet_ntoa( str_tmpIpAddr ), IP_ADDR_SIZE ); /* this is the answer */

    /* 
     * Fill the string tch_replySrcHwAddr with formatted MAC address contained in received datas
     * This is the hardware address of the sender of the parsed reply
     */
#define MAC(i) pstr_reply->tuc_senderHwAddr[(i)]
    snprintf( tch_replySrcHwAddr, MAC_ADDR_SIZE+1, "%02x:%02x:%02x:%02x:%02x:%02x", 
            MAC(0), MAC(1), MAC(2), MAC(3), MAC(4), MAC(5) );

    /* The same way, but with Hardware Address of the host we requested about (this must be the same than the one specified by user)) */
#define _MAC(i) pstr_reply->str_packet.tuc_targetHwAddr[(i)]
    snprintf( tch_replyHwAddr, MAC_ADDR_SIZE+1, "%02x:%02x:%02x:%02x:%02x:%02x", 
            _MAC(0), _MAC(1), _MAC(2), _MAC(3), _MAC(4), _MAC(5) );
    /* --- -- --- -- --- */

    return 0;
}


signed char footer( unsigned long ul_sentPackets, unsigned long ul_receivedPackets )
{
    signed char c_retValue;
    float f_lostProbesPercent;

    c_retValue = 0;

    /* Print out results if at least one request sent */
    if ( ul_sentPackets > 0 )
    {
        fprintf( stdout, "-- Rarping statistics --" );
        fprintf( stdout, "\nSent %ld probe(s)\n", ul_sentPackets );
        fprintf( stdout, "Received %ld response(s)\n", ul_receivedPackets );
		
        f_lostProbesPercent = 100 * ( ul_sentPackets - ul_receivedPackets ) / ul_sentPackets;
        fprintf( stdout, "Lost : %.2f%%\n", f_lostProbesPercent );
    }
    else
    {
        c_retValue = -1;
    }

    return c_retValue;
}


signed long openRawSocket( struct timeval str_timeout )
{
    signed long l_sock;

    l_sock = -1;

    /* Try to open Raw socket */
    if ( ( l_sock = socket( PF_PACKET, SOCK_RAW, htons( ETH_P_RARP ) ) ) < 0 ) /* ETH_P_ALL (or) RARP ??? */
    {
        perror( "socket" );
        if ( !IS_ROOT )
            fprintf( stderr, "Are you root?\n" );
    }		
    else
    {
        /* set the socket sending and receiving timeouts */
        setsockopt( l_sock, SOL_SOCKET, SO_RCVTIMEO, ( char * )&str_timeout, sizeof( str_timeout ) );
        setsockopt( l_sock, SOL_SOCKET, SO_SNDTIMEO, ( char * )&str_timeout, sizeof( str_timeout ) );
#ifdef DEBUG
        fprintf( stderr, "Timeout set to %lu ms\n", str_timeout.tv_sec * 1000 + str_timeout.tv_usec / 1000 );
#endif
    }
    /* Save the l_sock value to be able to close it as soon as required */
    l_SockRaw = l_sock;
	
    return l_sock;
}


signed char loop( const opt_t * pstr_argsDest, etherPacket_t * pstr_packet, struct sockaddr_ll * pstr_device, long l_socket )
{
    signed char c_retValue;
    unsigned char uc_received;
    unsigned long ul_noReply;
    struct timeval str_sendingMoment;

    ul_noReply = 0;
    c_retValue = 0;
    /* --- */
    while ( ( ul_NbProbes + 1 <= pstr_argsDest->ul_count ) || ( !( pstr_argsDest->ul_count ) ) )/* infinite loop if no count specified */
    {
        /* If we don't take care to the number of retries => jmp to the else, else check for actual number of retries */
        if ( ( pstr_argsDest->uc_unlimitedRetries ) ? 0 : ( ul_noReply == pstr_argsDest->ul_maximumRetries ) )
        {
            fprintf( stdout, "\nNo answer received after %lu tries\n", pstr_argsDest->ul_maximumRetries );
            c_retValue = -2;
            break;
        }
        else
        {
            if ( sendProbe( l_socket, pstr_packet, pstr_device ) != 1 )
            {
                fprintf( stderr, "Can't send request #%ld\n", ul_NbProbes++ );
            }
            else
            {
                ul_NbProbes++; /* Total probes sent */

                gettimeofday( &str_sendingMoment, NULL ); /* Record of sending timestamp */
#ifdef DEBUG
                fprintf( stderr, "Request #%ld sent\n", ul_NbProbes );
#endif
                /* the getAnswer function returns the number of replies received for each request (boolean value) */
                uc_received = getAnswer( l_socket, pstr_device, str_sendingMoment ); /* wait for an answer and parse it */
                ul_ReceivedReplies += uc_received; /* Total replies received */
                ul_noReply = ( uc_received ) ? 0 : ul_noReply + 1; /* Number of probes without answer */

                if ( pstr_argsDest->uc_exitOnReply && ul_ReceivedReplies )
                    break;
            }
            usleep( pstr_argsDest->ul_waitingMilliSeconds * 1000 ); /* Inter probes pause */
        }
    }

    return c_retValue;
}


signed char setTargetIpAddress( unsigned char * puc_targetIpAddress, const opt_t * pstr_argsDest )
{
    signed char c_retValue;
    /* struct in_addr just contains an unsigned long int */
    struct in_addr str_tmpAddr;

    c_retValue = 0;

    /* if RARP requests are choosen... */
    if ( pstr_argsDest->pch_IpAddrRarpReplies == NULL )
    {
        /* ... the field is set to 0 */
        memset( puc_targetIpAddress, 0, 4 );
    }
    else
    {
        /* inet_ntoa fills a given in_addr structure (using network byte order) with supplied IP address */
        if ( inet_aton( pstr_argsDest->pch_IpAddrRarpReplies, &str_tmpAddr ) == 0 )
        {
            fprintf( stderr, "Invalid IP address : %s\nUsing default 0.0.0.0\n", pstr_argsDest->pch_IpAddrRarpReplies );
            memset( puc_targetIpAddress, 0, 4 );
        }
        else
        {
            /* copy the processed IP address into the packet we'll send */
            memcpy( puc_targetIpAddress, &str_tmpAddr, sizeof( struct in_addr ) );
        }
    }

    return c_retValue;
}


signed char setSenderIpAddress( unsigned char * puc_senderIpAddress, const opt_t * pstr_argsDest, long l_socket )
{
    signed char c_retValue;
    /* struct in_addr just contains an unsigned long int */
    struct in_addr str_tmpAddr;
    struct ifreq str_tmpIfReq;

    c_retValue = 0;

    /* if user didn't choose to spoof his IP address */
    if ( pstr_argsDest->pch_spoofedLocalIpAddress == NULL )
    {
        /* ... the field is set with our real IP address */
        strncpy( str_tmpIfReq.ifr_name, pstr_argsDest->pch_iface, IFNAMSIZ-1 );

        if ( ioctl( l_socket, SIOCGIFADDR, &str_tmpIfReq ) < 0 )
        {
            perror( "ioctl" );
            fprintf( stderr, "Use default value : 0.0.0.0\n" );
            memset( puc_senderIpAddress, 0, 4 );
            c_retValue = 1;
        }
        else
        {
            struct sockaddr_in * pstr_access; /* provide a better control */
            pstr_access = ( struct sockaddr_in * )&str_tmpIfReq.ifr_addr;
           
            memcpy( puc_senderIpAddress, &pstr_access->sin_addr, 4 );
        }

    }
    else
    {
        /* inet_ntoa fills a given in_addr structure (using network byte order) with supplied IP address */
        if ( inet_aton( pstr_argsDest->pch_spoofedLocalIpAddress, &str_tmpAddr ) == 0 )
        {
            fprintf( stderr, "Invalid IP address : %s\nUsing default 0.0.0.0\n", pstr_argsDest->pch_spoofedLocalIpAddress );
            memset( puc_senderIpAddress, 0, 4 );
            c_retValue = 1;
        }
        else
        {
            /* copy the processed IP address into the packet we'll send */
            memcpy( puc_senderIpAddress, &str_tmpAddr, sizeof( struct in_addr ) );
        }
    }

    return c_retValue;  
}


struct timeval timeDiff( const struct timeval str_beginning, const struct timeval str_termination )
{
    struct timeval str_diff = { 0, 0 };

    /* We send the difference between first timestamp and second timestamp */
    str_diff.tv_sec  = str_termination.tv_sec  - str_beginning.tv_sec;
    str_diff.tv_usec = str_termination.tv_usec - str_beginning.tv_usec;

    return str_diff;
}


/* print out the content of a timeval structure as a floating
 * number of mmilliseconds */
void printTime_ms( const struct timeval str_time )
{
    /* Mike Muus Idol... */
    double triptime;

    triptime = ( ( double )str_time.tv_sec ) * 1000 + ( ( double )str_time.tv_usec ) / 1000;
    fprintf(stdout, "%.3f ms", triptime );
}


void rarpingOnExit( int sig )
{
    fprintf( stderr, "\nReceived signal : SIGINT (%d)\n", sig );
    /* Brief summary of what were sent/received */
    footer( ul_NbProbes, ul_ReceivedReplies );
    /* We close the socket before exiting */
    shutdown( l_SockRaw, SHUT_RDWR ); /* We are not vandals!! */
    close( l_SockRaw );
    exit( EXIT_FAILURE );
}

