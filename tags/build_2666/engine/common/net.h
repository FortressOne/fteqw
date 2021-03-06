/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net.h -- quake's interface to the networking layer

#define	PORT_ANY	-1

typedef enum {NA_INVALID, NA_LOOPBACK, NA_IP, NA_IPV6, NA_IPX, NA_BROADCAST_IP, NA_BROADCAST_IP6, NA_BROADCAST_IPX} netadrtype_t;

typedef enum {NS_CLIENT, NS_SERVER} netsrc_t;

typedef struct
{
	netadrtype_t	type;

	union {
		qbyte	ip[4];
		qbyte	ip6[16];
		qbyte	ipx[10];
	} address;

	unsigned short	port;
} netadr_t;

struct sockaddr_qstorage
{
	short dontusesa_family;
	unsigned char dontusesa_pad[6];
#if defined(_MSC_VER) || defined(MINGW)
	__int64 sa_align;
#else
	int sa_align[2];
#endif
	unsigned char sa_pad2[112];
};


extern	netadr_t	net_local_sv_ipadr;
extern	netadr_t	net_local_sv_ip6adr;
extern	netadr_t	net_local_sv_ipxadr;
extern	netadr_t	net_local_sv_tcpipadr;
extern	netadr_t	net_local_cl_ipadr;
extern	netadr_t	net_from;		// address of who sent the packet
extern	sizebuf_t	net_message;
//#define	MAX_UDP_PACKET	(MAX_MSGLEN*2)	// one more than msg + header
#define	MAX_UDP_PACKET	8192	// one more than msg + header
extern	qbyte		net_message_buffer[MAX_UDP_PACKET];

extern	cvar_t	hostname;

int TCP_OpenStream (netadr_t remoteaddr);	//makes things easier

void		NET_Init (void);
void		NET_InitClient (void);
void		NET_InitServer (void);
void		NET_CloseServer (void);
void UDP_CloseSocket (int socket);
void		NET_Shutdown (void);
qboolean	NET_GetPacket (netsrc_t socket);
void		NET_SendPacket (netsrc_t socket, int length, void *data, netadr_t to);

qboolean	NET_CompareAdr (netadr_t a, netadr_t b);
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b);
char		*NET_AdrToString (netadr_t a);
char		*NET_BaseAdrToString (netadr_t a);
qboolean	NET_StringToAdr (char *s, netadr_t *a);
qboolean NET_IsClientLegal(netadr_t *adr);

qboolean	NET_IsLoopBackAddress (netadr_t adr);

//============================================================================

#define	OLD_AVG		0.99		// total = oldtotal*OLD_AVG + new*(1-OLD_AVG)

#define	MAX_LATENT	32

typedef struct
{
	qboolean	fatal_error;

#ifdef NQPROT
	qboolean	isnqprotocol;
#endif

	float		last_received;		// for timeouts

// the statistics are cleared at each client begin, because
// the server connecting process gives a bogus picture of the data
	float		frame_latency;		// rolling average
	float		frame_rate;

	int			drop_count;			// dropped packets, cleared each level
	int			good_count;			// cleared each level

	netadr_t	remote_address;
	netsrc_t	sock;
	int			qport;

// bandwidth estimator
	double		cleartime;			// if realtime > nc->cleartime, free to go
//	double		rate;				// seconds / qbyte

// sequencing variables
	int			incoming_unreliable;	//dictated by the other end.
	int			incoming_sequence;
	int			incoming_acknowledged;
	int			incoming_reliable_acknowledged;	// single bit

	int			incoming_reliable_sequence;		// single bit, maintained local

	int			outgoing_unreliable;
	int			outgoing_sequence;
	int			reliable_sequence;			// single bit
	int			last_reliable_sequence;		// sequence number of last send

// reliable staging and holding areas
	sizebuf_t	message;		// writing buffer to send to server
	qbyte		message_buf[MAX_OVERALLMSGLEN];

	//nq has message truncation.
	int			reliable_length;
	int			reliable_start;
	qbyte		reliable_buf[MAX_OVERALLMSGLEN];	// unacked reliable message

// time and size data to calculate bandwidth
	int			outgoing_size[MAX_LATENT];
	double		outgoing_time[MAX_LATENT];
	qboolean	compress;

	//nq servers must recieve truncated packets.
	int in_fragment_length;
	char in_fragment_buf[MAX_OVERALLMSGLEN];
	int in_fragment_start;
} netchan_t;

extern	int	net_drop;		// packets dropped before this one

void Netchan_Init (void);
void Netchan_Transmit (netchan_t *chan, int length, qbyte *data, int rate);
void Netchan_OutOfBand (netsrc_t sock, netadr_t adr, int length, qbyte *data);
void VARGS Netchan_OutOfBandPrint (netsrc_t sock, netadr_t adr, char *format, ...);
void VARGS Netchan_OutOfBandTPrintf (netsrc_t sock, netadr_t adr, int language, translation_t text, ...);
qboolean Netchan_Process (netchan_t *chan);
void Netchan_Setup (netsrc_t sock, netchan_t *chan, netadr_t adr, int qport);

qboolean Netchan_CanPacket (netchan_t *chan, int rate);
qboolean Netchan_CanReliable (netchan_t *chan, int rate);
#ifdef NQPROT
qboolean NQNetChan_Process(netchan_t *chan);
#endif

#ifdef HUFFNETWORK
int Huff_PreferedCompressionCRC (void);
void Huff_EncryptPacket(sizebuf_t *msg, int offset);
void Huff_DecryptPacket(sizebuf_t *msg, int offset);
qboolean Huff_CompressionCRC(int crc);
void Huff_CompressPacket(sizebuf_t *msg, int offset);
void Huff_DecompressPacket(sizebuf_t *msg, int offset);
int Huff_GetByte(qbyte *buffer, int *count);
void Huff_EmitByte(int ch, qbyte *buffer, int *count);
#endif

#ifdef NQPROT
//taken from nq's net.h
//refer to that for usage info. :)

#define NETFLAG_LENGTH_MASK	0x0000ffff
#define NETFLAG_DATA		0x00010000
#define NETFLAG_ACK			0x00020000
#define NETFLAG_NAK			0x00040000
#define NETFLAG_EOM			0x00080000
#define NETFLAG_UNRELIABLE	0x00100000
#define NETFLAG_CTL			0x80000000

#define NET_GAMENAME_NQ		"QUAKE"
#define NET_PROTOCOL_VERSION	3


#define CCREQ_CONNECT		0x01
#define CCREQ_SERVER_INFO	0x02
#define CCREQ_PLAYER_INFO	0x03
#define CCREQ_RULE_INFO		0x04

#define CCREP_ACCEPT		0x81
#define CCREP_REJECT		0x82
#define CCREP_SERVER_INFO	0x83
#define CCREP_PLAYER_INFO	0x84
#define CCREP_RULE_INFO		0x85

//server->client protocol info
#define NQ_PROTOCOL_VERSION 15
#define DP5_PROTOCOL_VERSION 3502
#define DP6_PROTOCOL_VERSION 3503
#define DP7_PROTOCOL_VERSION 3504
#endif

int UDP_OpenSocket (int port, qboolean bcast);
int IPX_OpenSocket (int port, qboolean bcast);
void SockadrToNetadr (struct sockaddr_qstorage *s, netadr_t *a);
qboolean NET_Sleep(int msec, qboolean stdinissocket);
