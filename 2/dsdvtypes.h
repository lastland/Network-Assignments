/*
 * dsdvtypes.h
 *
 *  Created on: 2011-11-11
 *      Author: lastland
 */

#ifndef DSDVTYPES_H_
#define DSDVTYPES_H_

#define OK 0

// There are at most 6 machines.
#define MAX_MACHINE_NUM 6

// Name of every machine is upto 6.
#define MAX_NAME_LEN 6

typedef struct {
	// The name of this node. For example, a.
	char name;
	// The real machine name of this node. For example, head.
	char realname[MAX_NAME_LEN];
	// The number of its neighbors.
	int num_neighbors;
	// The number of reachable destinations.
	int num_des;
	// The port number.
	int port;
	// The sequence number to itself.
	int sequence;

	// The distance to its neighbors.
	double edge[MAX_MACHINE_NUM];
	// The distance to its destinations.
	double dist[MAX_MACHINE_NUM];
	// The sequence number to each destination.
	int sequences[MAX_MACHINE_NUM];
	// The names of this neighbor or destination.
	char names[MAX_MACHINE_NUM];
	// The real machine name of this neighbor or destination.
	char realnames[MAX_MACHINE_NUM][MAX_NAME_LEN];
	// A flag used to record whether this link is broken.
	int flags[MAX_MACHINE_NUM];
	// The ports of this neighbor or destination.
	int ports[MAX_MACHINE_NUM];

	// The next hop in routing table.
	char next_hop[MAX_MACHINE_NUM];
} NODE;

#endif /* DSDVTYPES_H_ */
