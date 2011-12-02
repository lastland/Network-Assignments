#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "rdt_struct.h"
#include "rdt_sender.h"
#include "rdt_utility.h"

//#define _LY_SENDER_DEBUG

unsigned int base;
unsigned int seq;
unsigned int packet_num;
const unsigned int zero = 0;

void send_packet_seq(void);

std::vector<packet> pkts;

int max;

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
#ifdef _LY_SENDER_DEBUG
    printf("Sender_Init()\n");
#endif
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
    base = 0;
    seq = 0;
    packet_num = 0;
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
#ifdef _LY_SENDER_DEBUG
    printf("Sender_FromUpperLayer(), base=%d, seq=%d\n", base, seq);
#endif
    /* split the message if it is too big */

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;

    char checksum[CHECKSUM_SIZE];
    
    while (msg->size-cursor > DATA_SIZE) {
	/* fill in the packet */
	packet pkt;
	do_checksum(checksum, msg->data+cursor,
			++packet_num, DATA_SIZE);
	make_pkt(&pkt, packet_num, msg->data+cursor,
			checksum, DATA_SIZE);
	pkts.push_back(pkt);

	/* move the cursor */
	cursor += DATA_SIZE;
    }

    /* send out the last packet */
    if (msg->size > cursor) {
	/* fill in the packet */
	packet pkt;
	do_checksum(checksum, msg->data+cursor, 
			++packet_num, msg->size-cursor);
	make_pkt(&pkt, packet_num, msg->data+cursor,
			checksum, msg->size-cursor);
	pkts.push_back(pkt);
    }
    send_packet_seq();

    max = (packet_num - base > max) ? packet_num - base : max;
    printf("%d\n", max);
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
#ifdef _LY_SENDER_DEBUG
	static int y = 0;
	if (seq - 1 == base && y == 0) {
    printf("Sender_FromLowerLayer(), base=%d, seq=%d\n", base, seq);
    y++;
	}
#endif
    int new_base;
    if ((new_base = is_ACK(pkt, (unsigned int)(base + 1))) >= 0) {
    	if (base + new_base > seq)
		return;
    	base += new_base + 1;
    	if (Sender_isTimerSet())
    		Sender_StopTimer();
    	Sender_StartTimer(TIMEOUT);
    	send_packet_seq();
    }
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
#ifdef _LY_SENDER_DEBUG
	static int x = 0;
	if (seq - 1 == base && x == 0) {
    printf("Sender_Timeout(), base=%d, seq=%d\n", base, seq);
    x++;
    }
#endif
    Sender_StartTimer(TIMEOUT);
    for (unsigned int i = base; i < seq; i++) {
    	packet pkt = pkts[i];
    	Sender_ToLowerLayer(&pkt);
    }
    if (base == seq)
    	if (Sender_isTimerSet())
    		Sender_StopTimer();
}

/* send the packet which has the seq index */
void send_packet_seq(void)
{
    if (seq < base + WINDOW_SIZE && seq < packet_num) {
    	packet pkt = pkts[seq];
    	Sender_ToLowerLayer(&pkt);

	if (base == seq)
		Sender_StartTimer(TIMEOUT);
	seq++;
    }
}
