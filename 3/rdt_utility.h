#ifndef _RDT_UTILITY_H_
#define _RDT_UTILITY_H_

#include "rdt_struct.h"

#define ACK 1

#define WINDOW_SIZE (6)
#define TIMEOUT (0.3)

/* The macros below defined the lengths of each block in the packet */
#define SEQ_SIZE 4
#define HEADER_SIZE 1
#define HEADER_AND_SEQ_SIZE (HEADER_SIZE + SEQ_SIZE)
#define SUM_TIMES 64
#define CHECKSUM_SIZE (RDT_PKTSIZE / SUM_TIMES)
#define DATA_SIZE (RDT_PKTSIZE - HEADER_AND_SEQ_SIZE - CHECKSUM_SIZE)
#define EXCEPT_CHECKSUM_SIZE (RDT_PKTSIZE - CHECKSUM_SIZE)

int is_ACK(struct packet *pkt, unsigned int seq);
bool not_corrupt(struct packet *pkt, int size);
bool has_seq_num(unsigned int seq, struct packet *pkt);
void do_checksum(char* checksum, char* given_data, unsigned int seq, int size);
void make_pkt(struct packet *pkt, unsigned int seq, char* data,
	char* checksum, int size);
void make_msg(struct message *msg, struct packet *pkt);

#endif  /* _RDT_UTILITY_H_ */
