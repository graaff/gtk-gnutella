/*
 * FILL_IN_EMILES_BLANKS
 *
 * Interface definition file.  One of the files that defines structures,
 * macros, etc. as part of the gui/core interface.
 *
 *----------------------------------------------------------------------
 * This file is part of gtk-gnutella.
 *
 *  gtk-gnutella is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  gtk-gnutella is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gtk-gnutella; if not, write to the Free Software
 *  Foundation, Inc.:
 *      59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *----------------------------------------------------------------------
 */

#ifndef _ui_core_interface_nodes_defs_h_
#define _ui_core_interface_nodes_defs_h_

#include "ui_core_interface_rx_defs.h"
#include "ui_core_interface_mq_defs.h"
#include "ui_core_interface_sq_defs.h"
#include "ui_core_interface_qrp_defs.h"
#include "ui_core_interface_hsep_defs.h"

/*
 * This structure keeps tracks of remote flow-control indications and
 * measures the time spent in flow-control over a period of time.  Every
 * half period we monitor the time we spend against a ratio.
 *
 * When no flow control occurs at all during two half periods, the structure
 * is disposed of.
 */

#define NODE_RX_FC_HALF_PERIOD	300		/* 5 minutes */

typedef enum node_protocol_types {
	PROTOCOL_TYPE_GNUTELLA = 0,
	PROTOCOL_TYPE_G2
} node_protocol_types_t;

struct node_rxfc_mon {
	time_t start_half_period;	/* When half period started */
	time_t fc_last_half;		/* Time spent in FC last half period */
	time_t fc_accumulator;		/* Time spent in FC this period */
	time_t fc_start;			/* Time when FC started, 0 if not in FC */
};

/*
 * MAX_CACHE_HOPS defines the maximum hop count we handle for the ping/pong
 * caching scheme.  Any hop count greater than that is thresholded to that
 * value.
 *
 * CACHE_HOP_IDX is the macro returning the array index in the
 * (0 .. MAX_CACHE_HOPS) range, based on the hop count.
 */
#define MAX_CACHE_HOPS	6		/* We won't handle anything larger */

#define CACHE_HOP_IDX(h)	(((h) > MAX_CACHE_HOPS) ? MAX_CACHE_HOPS : (h))

/*
 * Throttle periods for ping reception, depending on the peer mode.
 * This is applicable for regular pings, not alive pings.
 */
#define PING_REG_THROTTLE		3		/* seconds, regular peer */
#define PING_LEAF_THROTTLE		60		/* seconds, peer is leaf node */

typedef struct gnutella_node {
    gnet_node_t node_handle;    /* Handle of this node */
	node_peer_t peermode;		/* Operating mode (leaf, ultra, normal) */
	node_peer_t start_peermode;	/* Operating mode when handshaking begun */

	gchar error_str[256];		/* To sprintf() error strings with vars */
	struct gnutella_socket *socket;		/* Socket of the node */
	guint8 proto_major;			/* Handshaking protocol major number */
	guint8 proto_minor;			/* Handshaking protocol minor number */
	node_protocol_types_t protocol_type;

	guint8 qrp_major;			/* Query routing protocol major number */
	guint8 qrp_minor;			/* Query routing protocol minor number */
	guint8 uqrp_major;			/* UP Query routing protocol major number */
	guint8 uqrp_minor;			/* UP Query routing protocol minor number */
	gchar *vendor;				/* Vendor information */
	guchar vcode[4];			/* Vendor code (vcode[0] == NUL when unknown) */
	gpointer io_opaque;			/* Opaque I/O callback information */

	struct gnutella_header header;		/* Header of the current message */

	guint32 size;	/* How many bytes we need to read for the current message */

	gchar *data;			/* data of the current message */
	guint32 pos;			/* write position in data */

	guchar status;			/* See possible values below */
	guint32 flags;			/* See possible values below */
	guint32 attrs;			/* See possible values below */

	guint8 hops_flow;		/* Don't send queries with a >= hop count */
	guint8 max_ttl;			/* Value of their advertised X-Max-TTL */
	guint16 degree;			/* Value of their advertised X-Degree */

	GHashTable *qseen;			/* Queries seen from this leaf node */
	GHashTable *qrelayed;		/* Queries relayed from this node */
	GHashTable *qrelayed_old;	/* Older version of the `qrelayed' table */
	time_t qrelayed_created;	/* When `qrelayed' was created */

	guint32 sent;				/* Number of sent packets */
	guint32 received;			/* Number of received packets */
	guint32 tx_dropped;			/* Number of packets dropped at TX time */
	guint32 rx_dropped;			/* Number of packets dropped at RX time */
	guint32 n_bad;				/* Number of bad packets received */
	guint16 n_dups;				/* Number of dup messages received (bad) */
	guint16 n_hard_ttl;			/* Number of hard_ttl exceeded (bad) */
	guint32 n_weird;			/* Number of weird messages from that node */

	guint32 allocated;			/* Size of allocated buffer data, 0 for none */
	gboolean have_header;		/* TRUE if we have got a full message header */

	time_t last_update;			/* Last update of the node */
	time_t last_tx;				/* Last time we transmitted to the node */
	time_t last_rx;				/* Last time we received from the node */
	time_t connect_date;		/* When we got connected (after handshake) */
	time_t tx_flowc_date;		/* When we entered in TX flow control */
	struct node_rxfc_mon *rxfc;	/* Optional, time spent in RX flow control */
	time_t shutdown_date;		/* When we entered in shutdown mode */
	time_t up_date;				/* When remote server started (0 if unknown) */
	guint32 shutdown_delay;		/* How long we can stay in shutdown mode */

	const gchar *remove_msg;	/* Reason of removing */

	guint32 ip;					/* ip of the node */
	guint16 port;				/* port of the node */

	gchar *guid;				/* GUID of node (atom) for push-proxying */
	guint32 proxy_ip;			/* ip of the node for push proxyfication */
	guint16 proxy_port;			/* port of the node for push proxyfication */

	mqueue_t *outq;				/* TX Output queue */
	squeue_t *searchq;			/* TX Search queue */
	rxdrv_t *rx;				/* RX stack top */

	gpointer routing_data;		/* Opaque info, for gnet message routing */
	gpointer sent_query_table;	/* Opaque info, query table sent to node */
	gpointer recv_query_table;	/* Opaque info, query table recved from node */
	gpointer qrt_update;		/* Opaque info, query routing update handle */
	gpointer qrt_receive;		/* Opaque info, query routing reception */
	qrt_info_t *qrt_info;		/* Info about received query table */

	gpointer alive_pings;		/* Opaque info, for alive ping checks */
	time_t last_alive_ping;		/* Last time we sent an alive ping */
	guint alive_period;			/* Period for sending alive pings (secs) */

	wrap_buf_t hello;			/* Spill buffer for GNUTELLA HELLO */
	
	/*
	 * Data structures used by the ping/pong reduction scheme.
	 *		--RAM, 02/02/2002
	 */

	guint32 id;					/* Unique internal ID */
	guint ping_throttle;		/* Period for accepting new pings (secs) */
	time_t ping_accept;			/* Time after which we accept new pings */
	time_t next_ping;			/* When to send a ping, for "OLD" clients */
	gchar ping_guid[16];		/* The GUID of the last accepted ping */
	guchar pong_needed[MAX_CACHE_HOPS+1];	/* Pongs needed, by hop value */
	guchar pong_missing;		/* Sum(pong_needed[i]), i = 0..MAX_CACHE_HOPS */

	guint32 gnet_ip;			/* When != 0, we know the remote IP/port */
	guint16 gnet_port;			/* (listening port, that is ) */
	guint32 gnet_files_count;	/* Used to answer "Crawling" pings */
	guint32 gnet_kbytes_count;	/* Used to answer "Crawling" pings */
	guint32 gnet_pong_ip;		/* When != 0, last IP we got in pong */
	guint32 gnet_qhit_ip;		/* When != 0, last IP we got in query hit */
	gchar *gnet_guid;			/* GUID of node (atom) seen on the network */

	guint32 n_ping_throttle;	/* Number of pings we throttled */
	guint32 n_ping_accepted;	/* Number of pings we accepted */
	guint32 n_ping_special;		/* Number of special pings we received */
	guint32 n_ping_sent;		/* Number of pings we sent to this node */
	guint32 n_pong_received;	/* Number of pongs we received from this node */
	guint32 n_pong_sent;		/* Number of pongs we sent to this node */

	/*
	 * Traffic statistics -- RAM, 13/05/2002.
	 */

	gint32 tx_given;			/* Bytes fed to the TX stack (from top) */
	gint32 tx_deflated;			/* Bytes deflated by the TX stack */
	gint32 tx_written;			/* Bytes written by the TX stack */
	
	gint32 rx_given;			/* Bytes fed to the RX stack (from bottom) */
	gint32 rx_inflated;			/* Bytes inflated by the RX stack */
	gint32 rx_read;				/* Bytes read from the RX stack */

	/*
	 * Various Gnutella statistics -- RAM, 10/12/2003.
	 *
	 * qrp_queries/qrp_matches is used by both leaf and ultra nodes:
	 * . Leaf structures use it to count the amount of queries received versus
	 *   queries sent after QRP filtering by the ultra node (us).
	 * . Ultra structures use it to count the amount of queries received from
	 *   the ultra node by the leaf node (us) versus the amount of queries
	 *   that really caused a match to one of our files.
	 */

	guint32 qrp_queries;		/* Queries received under QRP control */
	guint32 qrp_matches;		/* Queries received that incurred a match */
	guint32 rx_queries;			/* Total amount of queries received */
	guint32 tx_queries;			/* Total amount of queries sent */
	guint32 rx_qhits;			/* Total amount of hits received */
	guint32 tx_qhits;			/* Total amount of hits sent */

	/*
	 * Horizon size estimation (HSEP) -- TSC, 11/02/2004.
	 */

	hsep_triple hsep_table[HSEP_N_MAX+1];        /* Connection's HSEP table */
	hsep_triple hsep_sent_table[HSEP_N_MAX];     /* Previous table sent */
	guint32 hsep_msgs_received;                  /* # of msgs received */
	guint32 hsep_triples_received;               /* # of triples received */
	time_t hsep_last_received;                   /* When last msg was rcvd */
	guint32 hsep_msgs_sent;                      /* # of msgs sent */
	guint32 hsep_triples_sent;                   /* # of triples sent */
	time_t hsep_last_sent;                       /* When last msg was sent */
} gnutella_node_t;

/*
 * Node flags.
 */

#define NODE_F_HDSK_PING	0x00000001	/* Expecting handshake ping */
#define NODE_F_STALE_QRP	0x00000002	/* Is sending a stale QRP patch */
#define NODE_F_INCOMING		0x00000004	/* Incoming (permanent) connection */
#define NODE_F_ESTABLISHED	0x00000008	/* Gnutella connection established */
#define NODE_F_VALID		0x00000010	/* We handshaked with a Gnutella node */
#define NODE_F_ALIEN_IP		0x00000020	/* Pong-IP does not match TCP/IP addr */
#define NODE_F_WRITABLE		0x00000040	/* Node is writable */
#define NODE_F_READABLE		0x00000080	/* Node is readable, process queries */
#define NODE_F_BYE_SENT		0x00000100	/* Bye message was queued */
#define NODE_F_NODELAY		0x00000200	/* TCP_NODELAY was activated */
#define NODE_F_NOREAD		0x00000400	/* Prevent further reading from node */
#define NODE_F_EOF_WAIT		0x00000800	/* During final shutdown, waiting EOF */
#define NODE_F_CLOSING		0x00001000	/* Initiated bye or shutdown */
#define NODE_F_ULTRA		0x00002000	/* Is one of our ultra nodes */
#define NODE_F_LEAF			0x00004000	/* Is one of our leaves */
#define NODE_F_CRAWLER		0x00008000	/* Is a Gnutella Crawler */
#define NODE_F_FAKE_NAME	0x00010000	/* Was unable to validate GTKG name */
#define NODE_F_PROXY		0x00020000	/* Sent a push-proxy request */
#define NODE_F_QRP_SENT		0x00040000	/* Undergone one complete QRP sending */
#define NODE_F_TLS			0x00080000	/* TLS-tunneled */

/*
 * Node attributes.
 */

#define NODE_A_BYE_PACKET	0x00000001	/* Supports Bye-Packet */
#define NODE_A_PONG_CACHING	0x00000002	/* Supports Pong-Caching */
#define NODE_A_PONG_ALIEN	0x00000004	/* Alien Pong-Caching scheme */
#define NODE_A_QHD_NO_VTAG	0x00000008	/* Servent has no vendor tag in QHD */
#define NODE_A_RX_INFLATE	0x00000010	/* Reading compressed data */
#define NODE_A_TX_DEFLATE	0x00000020	/* Sending compressed data */
#define NODE_A_ULTRA		0x00000040	/* Node wants to be an Ultrapeer */
#define NODE_A_NO_ULTRA		0x00000080	/* Node is NOT ultra capable */
#define NODE_A_UP_QRP		0x00000100	/* Supports intra-UP QRP */
#define NODE_A_LEAF_GUIDE	0x00000200	/* Supports leaf-guided dyn queries */

#define NODE_A_CAN_HSEP		0x04000000	/* Node supports HSEP */
#define NODE_A_CAN_QRP		0x08000000	/* Node supports query routing */
#define NODE_A_CAN_VENDOR	0x10000000	/* Node supports vendor messages */
#define NODE_A_CAN_GGEP		0x20000000	/* Node supports big pongs, etc.. */
#define NODE_A_CAN_ULTRA	0x40000000	/* Node is ultra capable */
#define NODE_A_CAN_INFLATE	0x80000000	/* Node capable of inflating */


/*
 * State inspection macros.
 */

#define NODE_IS_CONNECTING(n)						\
	(	(n)->status == GTA_NODE_CONNECTING			\
	||	(n)->status == GTA_NODE_HELLO_SENT			\
	||	(n)->status == GTA_NODE_WELCOME_SENT		\
	||	(n)->status == GTA_NODE_RECEIVING_HELLO	)

#define NODE_IS_CONNECTED(n)						\
	(	(n)->status == GTA_NODE_CONNECTED			\
	||	(n)->status == GTA_NODE_SHUTDOWN )

#define NODE_IS_INCOMING(n)	\
	((n)->flags & NODE_F_INCOMING)

#define NODE_IS_REMOVING(n) \
	((n)->status == GTA_NODE_REMOVING)

#define NODE_IN_TX_FLOW_CONTROL(n) \
	((n)->outq && mq_is_flow_controlled((n)->outq))

#define NODE_IN_TX_SWIFT_CONTROL(n) \
	((n)->outq && mq_is_swift_controlled((n)->outq))

#define NODE_IS_WRITABLE(n) \
	((n)->flags & NODE_F_WRITABLE)

#define NODE_IS_READABLE(n) \
	(((n)->flags & (NODE_F_READABLE|NODE_F_NOREAD)) == NODE_F_READABLE)

#define NODE_IS_ESTABLISHED(n) \
	(((n)->flags & (NODE_F_WRITABLE|NODE_F_ESTABLISHED)) == \
		(NODE_F_WRITABLE|NODE_F_ESTABLISHED))

#define NODE_MQUEUE_PERCENT_USED(n) \
	((n)->outq ? mq_size((n)->outq) * 100 / mq_maxsize((n)->outq) : 0)

#define NODE_SQUEUE(n) ((n)->searchq)

#define NODE_MQUEUE_COUNT(n) \
	((n)->outq ? mq_count((n)->outq) : 0)

#define NODE_MQUEUE_PENDING(n) \
	((n)->outq ? mq_pending((n)->outq) : 0)

#define NODE_MQUEUE_ABOVE_LOWAT(n) \
	((n)->outq ? mq_size((n)->outq) > mq_lowat((n)->outq) : FALSE)

#define NODE_SQUEUE_COUNT(n) \
	((n)->searchq ? sq_count((n)->searchq) : 0)

#define NODE_SQUEUE_SENT(n) \
	((n)->searchq ? sq_sent((n)->searchq) : 0)

#define NODE_RX_COMPRESSED(n) \
	((n)->attrs & NODE_A_RX_INFLATE)

#define NODE_TX_COMPRESSED(n) \
	((n)->attrs & NODE_A_TX_DEFLATE)

#define NODE_TX_COMPRESSION_RATIO(n)	\
	((n)->tx_given ?					\
		(double) ((n)->tx_given - (n)->tx_deflated) / (n)->tx_given : 0.0)

#define NODE_RX_COMPRESSION_RATIO(n)	\
	((n)->rx_inflated ?					\
		(double) ((n)->rx_inflated - (n)->rx_given) / (n)->rx_inflated : 0.0)

#define NODE_ID(n)				((n)->id)

#define NODE_CAN_GGEP(n)		((n)->attrs & NODE_A_CAN_GGEP)
#define NODE_UP_QRP(n)			((n)->attrs & NODE_A_UP_QRP)
#define NODE_GUIDES_QUERY(n)	((n)->attrs & NODE_A_LEAF_GUIDE)

/*
 * Peer inspection macros
 */

#define NODE_IS_LEAF(n)			((n)->peermode == NODE_P_LEAF)
#define NODE_IS_NORMAL(n)		((n)->peermode == NODE_P_NORMAL)
#define NODE_IS_ULTRA(n)		((n)->peermode == NODE_P_ULTRA)
#define NODE_IS_UDP(n)			((n)->peermode == NODE_P_UDP)

/*
 * Macros.
 */

#define node_vendor(n)		((n)->vendor != NULL ? (n)->vendor : "????")

#define node_inc_sent(n)            node_add_sent(n, 1)
#define node_inc_txdrop(n)          node_add_txdrop(n, 1)
#define node_inc_rxdrop(n)          node_add_rxdrop(n, 1)

#define node_add_tx_given(n,x)		do { (n)->tx_given += (x); } while (0)
#define node_add_tx_written(n,x)	do { (n)->tx_written += (x); } while (0)
#define node_add_tx_deflated(n,x)	do { (n)->tx_deflated += (x); } while (0)

#define node_add_rx_given(n,x)		do { (n)->rx_given += (x); } while (0)
#define node_add_rx_inflated(n,x)	do { (n)->rx_inflated += (x); } while (0)
#define node_add_rx_read(n,x)		do { (n)->rx_read += (x); } while (0)

#define node_inc_tx_query(n)		do { (n)->tx_queries++; } while (0)
#define node_inc_rx_query(n)		do { (n)->rx_queries++; } while (0)
#define node_inc_tx_qhit(n)			do { (n)->tx_qhits++; } while (0)
#define node_inc_rx_qhit(n)			do { (n)->rx_qhits++; } while (0)

#define node_inc_qrp_query(n)		do { (n)->qrp_queries++; } while (0)
#define node_inc_qrp_match(n)		do { (n)->qrp_matches++; } while (0)

/*
 * Check whether Ultra node has received our QRP table, or whether
 * we fully got the QRP table from the leaf.
 */
#define node_ultra_received_qrp(n) \
	(NODE_IS_ULTRA(n) && \
	(n)->qrt_update == NULL && (n)->sent_query_table != NULL)
#define node_leaf_sent_qrp(n) \
	(NODE_IS_LEAF(n) && \
	(n)->qrt_receive == NULL && (n)->recv_query_table != NULL)

/*
 * Can we send query with hop count `h' according to node's hops-flow value?
 */
#define node_query_hops_ok(n, h)	((h) < (n)->hops_flow)

/* Don't include "routing.h" just for that routine */
extern gboolean route_exists_for_reply(gchar *muid, guint8 function);

/*
 * Can we send message of type `t', bearing hop count `h' and MUID `m'?
 *
 * For queries, we look at the hops-flow, and whether there is a route for
 * the query hit: no need to forward the request if the reply will be dropped.
 * (we always forward queries with hops=0, of course, since they are ours!).
 */
#define node_can_send(n, t, h, m) \
	((t) != GTA_MSG_SEARCH || \
		(node_query_hops_ok(n, h) && \
			((h) == 0 || route_exists_for_reply(m, t))))

/*
 * node_flowc_swift_grace
 * node_flowc_swift_period
 *
 * The grace period between the time the node enters flow-control and the
 * time we want to speed up things and drop traffic, entering "swift" mode.
 * For a leaf node, we allow more time before we start to aggressively drop
 * traffic, but for a peer, we need to respond quickly, to avoid long clogging.
 *
 * In "swift" mode, a callback is periodically invoked to drop more traffic
 * if we don't see much progress in the queue backlog.  For a leaf node, it
 * is invoked less often than for a peer.
 * 
 */
#define node_flowc_swift_grace(n)	(NODE_IS_LEAF(n) ? 210 : 30)
#define node_flowc_swift_period(n)	(NODE_IS_LEAF(n) ? 140 : 20)

/*
 * Global Data
 */

#define GNUTELLA_HELLO "GNUTELLA CONNECT/"
#define GNUTELLA_HELLO_LENGTH	(sizeof(GNUTELLA_HELLO) - 1)

#define NODE_ID_LOCAL	0x0U		/* ID for "local node" (ourselves) */

	
#endif

/* vi: set ts=4 sw=4 cindent: */
