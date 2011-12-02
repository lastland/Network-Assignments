/*
 * dsdvmsgmgr.c
 *
 *  Created on: 2011-11-11
 *      Author: lastland
 */

#include <stdio.h>
#include <string.h>
#include "dsdvtypes.h"
#include "dsdvmsgmgr.h"

#define BUFLEN_EACH (100)
#define BUFLEN_ALL (BUFLEN_EACH * MAX_MACHINE_NUM)

void node_to_msg(NODE* node, char* msg) {
	int i;
	char buf[BUFLEN_EACH];
	sprintf(msg, "%c %d %d", node->name,
			node->num_des,
			node->sequence);
	for (i = 0; i < node->num_des; i++) {
		sprintf(buf, " %c %d %.1lf",
				node->names[i],
				node->sequences[i],
				node->dist[i]);
		strcat(msg, buf);
	}
}

void msg_to_node(char* msg, NODE* node) {
	int i;
	char* buf;
	char tmp[BUFLEN_ALL];
	node->name = strtok(msg, " ")[0];
	buf = strtok(NULL, " ");
	sscanf(buf, "%d", &node->num_neighbors);
	buf = strtok(NULL, " ");
	sscanf(buf, "%d", &node->sequence);
	for (i = 0; i < node->num_neighbors; i++) {
		node->names[i] = strtok(NULL, " ")[0];
		buf = strtok(NULL, " ");
		sscanf(buf, "%d", &node->sequences[i]);
		buf = strtok(NULL, " ");
		sscanf(buf, "%lf", &node->dist[i]);
	}
	node->num_des = node->num_neighbors;
}
