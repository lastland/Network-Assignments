#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_utility.h"
#include "rdt_receiver.h"

//#define _LY_RECEIVER_DEBUG

unsigned int exp_seq;
struct packet sndpkt;
char ack;
char checksum[CHECKSUM_SIZE];

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    ack = ACK;
    exp_seq = 0;

    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
    do_checksum(checksum, &ack, exp_seq, 1);
    make_pkt(&sndpkt, exp_seq++, &ack, checksum, 1);
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
	if (!pkt->data[0]) return;
    if (!(not_corrupt(pkt, pkt->data[0]) && has_seq_num(exp_seq, pkt))) {
#ifdef _LY_RECEIVER_DEBUG
    	unsigned int rec_seq;
    	memcpy(&rec_seq, pkt->data+HEADER_SIZE, SEQ_SIZE);
    	printf("Not OK!exp_seq=%d, rec_seq=%d\n", exp_seq, rec_seq);
#endif
    	Receiver_ToLowerLayer(&sndpkt);
	return;
    }

    /* construct a message and deliver to the upper layer */
#ifdef _LY_RECEIVER_DEBUG
    printf("Receive seq#%d\n", exp_seq);
#endif
    struct message msg;
    char data[DATA_SIZE];
    msg.size = pkt->data[0];
    msg.data = data;
    ASSERT(msg.data!=NULL);
    make_msg(&msg, pkt);

    /* sanity check in case the packet is corrupted */
    if (msg.size<0) msg.size=0;
    if (msg.size>RDT_PKTSIZE-HEADER_SIZE) msg.size=RDT_PKTSIZE-HEADER_SIZE;

    Receiver_ToUpperLayer(&msg);
    do_checksum(checksum, &ack, exp_seq, 1);
    make_pkt(&sndpkt, exp_seq++, &ack, checksum, 1);
    Receiver_ToLowerLayer(&sndpkt);
}
