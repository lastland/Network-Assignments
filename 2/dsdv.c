/*
 * dsdv.c
 *
 *  Created on: 2011-11-11
 *      Author: Li Yao
 *
 *  Edit:
 *  2011-11-12	Fix some bugs.	Li Yao
 *
 *  This is a program written for my network assignemnt.
 *  Socket and pthread and many other stuff are used in this program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <memory.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "dsdvtypes.h"
#include "dsdvmsgmgr.h"

#define BUFLEN 1000

/*
 * The current information of this node.
 * This includes many stuff like routing table and so on.
 * You can check the definition of NODE if dsdvtypes.h.
 */
NODE node;

/*
 * The read and write lock which would be used when a thread
 * is about to update information of node.
 */
pthread_mutex_t mutex;

char* filename;
FILE* file;

/* 
 * Initial the node. 
 */
void init_node(NODE* node) {
	int i;
	node->num_neighbors = 0;
	node->port = -1;
	node->sequence = 0;
	for (i = 0; i < MAX_MACHINE_NUM; i++) {
		node->edge[i] = -1;
		node->dist[i] = -1;
		node->sequences[i] = 0;
		node->ports[i] = -1;
		node->flags[i] = 0;
	}
}

/*
 * Maintain the node information.
 * This function should be used every time when an update
 * happens in routing table.
 */
void maintain_node(NODE* node) {
	int i;
	for (i = 0; i < node->num_neighbors; i++) {
		if (node->next_hop[i] == node->names[i]) {
			node->dist[i] = node->edge[i];
		}
		// In case a link is broken.
		if (0 > node->edge[i]) {
			if (!node->flags[i]) {
				node->dist[i] = node->edge[i];
				node->sequences[i] += 1;
				node->flags[i] = 1;
			}
		} else if (node->flags[i]) {
			node->flags[i] = 0;
		}
	}
}

/*
 * Initial the node from the specified node.
 */
int init_from_file() {
	int i;
	init_node(&node);
	if ((file = fopen(filename, "r")) == NULL) {
		return -1;
	}
	fscanf(file, "%d %c\n", &node.num_neighbors, &node.name);
	for (i = 0; i < node.num_neighbors; i++) {
		fscanf(file, "%c %s %lf %d\n",
				&node.names[i],
				node.realnames[i],
				&node.edge[i],
				&node.ports[i]);
		node.dist[i] = node.edge[i];
		node.next_hop[i] = node.names[i];
	}
	node.num_des = node.num_neighbors;

	fclose(file);
	return OK;
}

/*
 * Check if there is any updates in routing table.
 * This function would reopen the specified file every time it
 * was executed, and call maintain_node before its end.
 */
void check_updates() {
	int tmpi, i, j, tj;
	double tmpd;
	char tmpc;
	char tmps[MAX_NAME_LEN];
	file = fopen(filename, "r");
	fscanf(file, "%d %c\n", &tmpi, &tmpc);
	for (i = 0; i < node.num_neighbors; i++) {
		fscanf(file, "%c %s %lf %d\n",
				&tmpc,
				tmps,
				&tmpd,
				&tmpi);
		for (j = i; j < node.num_neighbors + i; j++) {
			tj = j >= node.num_neighbors ?
					j - node.num_neighbors : j;
			if (node.names[tj] == tmpc) {
				node.edge[tj] = tmpd;
				break;
			}
		}
	}
	fclose(file);
	maintain_node(&node);
}

/*
 * Translate the message received from other nodes and update 
 * the present routing table according to this message.
 * This function uses the msg_to_node function declared in 
 * dsdvmsgmgr.h and defined in dsdvmsgmgr.c.
 */
void translate_and_update(char* msg) {
	int i, j, k;
	int flag = 0;
	NODE another_node;

	init_node(&another_node);
	msg_to_node(msg, &another_node);
	// Locate the number of the node received in current routing table.
	for (k = 0; k < node.num_neighbors; k++) {
		if (node.names[k] == another_node.name)
			break;
	}
	// The following code will update the routing table according to sequence and other info.
	if (node.sequences[k] < another_node.sequence) {
		check_updates();
		node.sequences[k] = another_node.sequence;
		node.dist[k] = node.edge[k];
		node.next_hop[k] = another_node.name;
	}
	for (i = 0; i < another_node.num_des; i++) {
		if (another_node.names[i] == node.name)
			continue;
		flag = 0;
		for (j = 0; j < node.num_des; j++) {
			if (another_node.names[i] == node.names[j]) {
				flag = 1;
				if (another_node.sequences[i] > node.sequences[j]) {
					node.sequences[j] = another_node.sequences[i];
					if (0 > another_node.dist[i] || 0 > node.edge[k]) {
						// If a link is broken.
						node.dist[j] = -1;
					} else {
						node.dist[j] = another_node.dist[i] + node.edge[k];
						node.next_hop[j] = another_node.name;
					}
				} else if (another_node.sequences[i] == node.sequences[j]){
					if (!(0 > another_node.dist[i] || 0 > node.edge[k]) &&
							another_node.dist[i] + node.edge[k] < node.dist[j] ||
							0 > node.dist[j]) {
						// There is a shorter path.
						node.dist[j] = another_node.dist[i] + node.edge[k];
						node.next_hop[j] = another_node.name;
					}
				}
				break;
			}
		}
		// If a reachable destination is not in current routing table.
		if (!flag) {
			node.dist[node.num_des] = node.edge[k] + another_node.dist[i];
			node.names[node.num_des] = another_node.names[i];
			node.next_hop[node.num_des++] = another_node.name;
		}
	}
}

/*
 * This function would bind it to the specified port and wait
 * until there is a message received. After receiving something,
 * it would call translate_and_update to do updates.
 * This is a function started by a new thread.
 */
void* recv_and_update(void* t) {
	int tid = (int) t;
	int sockfd;
	struct sockaddr_in addr;
	int numbytes;
	char buf[BUFLEN];
	int flag = 1, len = sizeof(int);

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Failed to create socket on thread %d.\n", tid);
		exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons( node.port );
	addr.sin_addr.s_addr = htonl( INADDR_ANY );

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, len);
	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr))) {
		printf("Failed to bind socket on thread %d.\n", tid);
		exit(-1);
	}

	// Receive and update. This loop is supposed to run forever.
	while (1) {
		if ((numbytes = recv(sockfd, buf, BUFLEN, 0)) < 0) {
			printf("Failed to receive msgs on thread %d.\n",
					tid);
			exit(-1);
		}
		buf[numbytes] = '\0';
		pthread_mutex_lock(&mutex);
		translate_and_update(buf);
		pthread_mutex_unlock(&mutex);
	}

	close(sockfd);
	pthread_exit(NULL);
}

/*
 * This function would check updates through check_updates function,
 * and then broadcast the present routing table every two seconds.
 * It is also responsible for increasing the sequence number every
 * ten seconds.
 */
void* send_updates(void* t) {
	int tid = (int) t;
	int sockfd;
	struct sockaddr_in addr[MAX_MACHINE_NUM];
	char buf[BUFLEN];
	struct in_addr **pptr;
	struct hostent* ht;
	static int print_out_number = 0;

	int i, j = 0;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Failed to create socket on thread %d.\n", tid);
		exit(-1);
	}

	for (i = 0; i < node.num_neighbors; i++) {
		if ((ht = gethostbyname(node.realnames[i])) == NULL) {
			printf("Failed to resolve the hostname on thread %d.\n", tid);
			exit(-1);
		}
		pptr = (struct in_addr **) ht->h_addr_list;
		memset(&addr[i], 0, sizeof(addr[i]));
		addr[i].sin_family = AF_INET;
		addr[i].sin_port = htons(node.ports[i]);
		memcpy(&addr[i].sin_addr, *pptr, sizeof(struct in_addr));
	}

	while (1) {
		j++;
		pthread_mutex_lock(&mutex);
		check_updates();
		// Transfer the information of this node to a string.
		node_to_msg(&node, buf);
		// Print the current routing table.
		printf("## print-out number %d\n", ++print_out_number);
		for (i = 0; i < node.num_des; i++) {
			printf("shortest path to node %c (seq# %d): the next hop is %c and the cost is %.1lf\n",
					node.names[i],
					node.sequences[i],
					node.next_hop[i],
					node.dist[i]);
		}
		pthread_mutex_unlock(&mutex);

		// Broadcase the information of this node.
		for (i = 0; i < node.num_neighbors; i++) {
			if (0 > node.edge[i])
				continue;
			if (sendto(sockfd, buf, strlen(buf), 0,
					(struct sockaddr*)&addr[i],
					sizeof(addr[i])) < 0) {
				printf("Failed to send msgs on thread %d.\n",
						tid);
				exit(-1);
			}
		}

		// This thread would sleep for 2 seconds.
		sleep(2);

		pthread_mutex_lock(&mutex);
		// Sequence change only happens every 10 seconds.
		if (j % 5 == 0) {
			node.sequence += 2;
		}
		pthread_mutex_unlock(&mutex);
	}

	close(sockfd);
	pthread_exit(NULL);
}

/*
 * The main function.
 */
int main(int argc, char** argv) {
	pthread_t recvt;
	pthread_t sendt;
	pthread_attr_t attr;
	int recvtid = 1;
	int sendtid = 2;

	// In case wrong arguments is given for this program.
	if (argc != 3) {
		printf("Usage: %s portnumber filename\n", argv[0]);
		exit(1);
	}
	filename = argv[2];
	// In case the file doesn't exist.
	if (init_from_file() != OK) {
		printf("File %s doesn't exist!\n", filename);
		exit(-1);
	}
	node.port = atoi(argv[1]);

	// Initial some pthread stuff.
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&mutex, NULL);

	// Create receiving and sending threads.
	pthread_create(&recvt, &attr, recv_and_update, (void*) recvtid);
	pthread_create(&sendt, &attr, send_updates, (void*) sendtid);

	pthread_attr_destroy(&attr);

	// The main thread will wait for other two threads.
	pthread_join(recvt, NULL);
	pthread_join(sendt, NULL);
	// The mutex lock will not be destroyed if other two threads is still running.
	pthread_mutex_destroy(&mutex);
	pthread_exit(NULL);
	return 0;
}
