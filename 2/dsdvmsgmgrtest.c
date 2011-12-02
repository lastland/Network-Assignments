#include <stdio.h>
#include <string.h>
#include "dsdvtypes.h"
#include "dsdvmsgmgr.h"

void main() {
	NODE node;
	int i;
	char msg[100];

	node.name = 'a';
	node.realname;
	strcpy(node.realname, "head");
	node.num_neighbors = 1;
	for (i = 0; i < node.num_neighbors; i++) {
		node.names[i] = 'b' + i;
		node.dist[i] = 1;
		node.sequences[i] = 0;
	}
	node_to_msg(&node, msg);
	printf("%s\n", msg);
	msg_to_node(msg, &node);
	node_to_msg(&node, msg);
	printf("%s\n", msg);
}
