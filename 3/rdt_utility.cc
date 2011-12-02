#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdt_utility.h"

/* is the packet an ACK packet? return 0 or a postive number if it is */
int is_ACK(struct packet *pkt, unsigned int seq)
{
	bool flag = true;
	unsigned int rec_seq;
	char ack, rec_checksum[CHECKSUM_SIZE], exp_checksum[CHECKSUM_SIZE];
	memset(exp_checksum, 0, CHECKSUM_SIZE);
	memset(rec_checksum, 0, CHECKSUM_SIZE);
	memcpy(&rec_seq, pkt->data+HEADER_SIZE, SEQ_SIZE);
	memcpy(&ack, pkt->data+HEADER_SIZE+SEQ_SIZE, 1);
	memcpy(rec_checksum, pkt->data+HEADER_SIZE+SEQ_SIZE+DATA_SIZE, CHECKSUM_SIZE);
	do_checksum(exp_checksum, &ack, rec_seq, 1);
	for (int i = 0; i < CHECKSUM_SIZE; i++) {
		if (exp_checksum[i] != rec_checksum[i]) {
			flag = false;
			break;
		}
	}
	int result = (flag && ACK == ack) ? rec_seq - seq : -1;
	return result;
}

/* judge whether the packet is corrupted */
bool not_corrupt(struct packet *pkt, int size)
{
#ifdef _LY_UTILITY_DEBUG
	printf("size=%d\n", size);
#endif
	if (size <= 0 || size > DATA_SIZE) return false;
	bool result = true;
	char exp_checksum[CHECKSUM_SIZE];
	char rec_checksum[CHECKSUM_SIZE];
	char rec_data[size];
	unsigned int rec_seq;
	memset(rec_data, 0, size);
	memcpy(&rec_seq, pkt->data+HEADER_SIZE, SEQ_SIZE);
	memcpy(rec_checksum, pkt->data+HEADER_SIZE+SEQ_SIZE+DATA_SIZE, CHECKSUM_SIZE);
	memcpy(rec_data, pkt->data+HEADER_SIZE+SEQ_SIZE, size);
	do_checksum(exp_checksum, rec_data, rec_seq, size);
	for (int i = 0; i < CHECKSUM_SIZE; i++) {
		if (exp_checksum[i] != rec_checksum[i]) {
			result = false;
			break;
		}
	}
	return result;
}

/* judge whether the given packet is of the given sequence number */
bool has_seq_num(unsigned int seq, struct packet *pkt)
{
	unsigned int exp_seq;
	memcpy(&exp_seq, pkt->data+HEADER_SIZE, SEQ_SIZE);
	return exp_seq == seq;
}

/* do a checksum algorithm, store the result in the firts parameter */
void do_checksum(char* checksum, char* given_data, unsigned int seq, int size)
{
	int cursor = 0;
	short tmp;
	short advance = 0;
	bool flag = false;
	char data[EXCEPT_CHECKSUM_SIZE];

	memset(checksum, 0, CHECKSUM_SIZE);
	memset(data, 0, sizeof(data));
	memcpy(data, &size, HEADER_SIZE);
	memcpy(data+HEADER_SIZE, &seq, SEQ_SIZE);
	memcpy(data+HEADER_AND_SEQ_SIZE, given_data, size);

	for (int i = 0; i < SUM_TIMES - 1; i++) {
		int j = cursor;
		flag = false;
		while (1 == advance || !flag) {
			tmp = checksum[j - cursor] + data[j] + advance;
			advance = (tmp > 511);
			checksum[j - cursor] += data[j] + advance;
			data[j] = 0;
			if (cursor + CHECKSUM_SIZE == ++j) {
				j -= CHECKSUM_SIZE;
				flag = true;
			}
		}
		cursor += CHECKSUM_SIZE;
	}
	for (int i = 0; i < CHECKSUM_SIZE; i++)
		checksum[i] = ~checksum[i];
}

/* make a packet from the given datas, store the result in the first parameter */
void make_pkt(struct packet* pkt, unsigned int seq, char* data, char* checksum, int size)
{
	memset(pkt, 0, sizeof(packet));
	memcpy(pkt->data, &size, HEADER_SIZE);
	memcpy(pkt->data+HEADER_SIZE, &seq, SEQ_SIZE);
	memcpy(pkt->data+HEADER_SIZE+SEQ_SIZE, data, size);
	memcpy(pkt->data+HEADER_SIZE+SEQ_SIZE+DATA_SIZE, checksum, CHECKSUM_SIZE);
}

/* make a message from the given packet, store the result in the first paramter */
void make_msg(struct message *msg, struct packet *pkt)
{
	msg->size = pkt->data[0];
	memcpy(msg->data, pkt->data+HEADER_SIZE+SEQ_SIZE, DATA_SIZE);
}
