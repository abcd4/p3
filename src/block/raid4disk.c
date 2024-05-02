/* Author: Robbert van Renesse, November 2023
 *
 * This block store module implements RAID4.
 *
 *		block_if raid4disk_init(block_if *below, unsigned int nbelow){
 *			'below' is an array of underlying block stores, all of which
 *			are assumed to be of the same size.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <egos/block_store.h>

// Disk nbelow-1 is used as the parity disk
struct raid4disk_state {
	block_if *below;		// block stores below
	unsigned int nbelow;	// #block stores
};

static int raid4disk_getninodes(block_if bi){
	return 1;
}

static int raid4disk_getsize(block_if bi, unsigned int ino){
	if (ino != 0) {
		fprintf(stderr, "!!raid4disk_getsize: ino != 0 not supported\n");
		return -1;
	}
    // Your code goes here
    return -1;
}

static int raid4disk_setsize(block_if bi, unsigned int ino, block_no nblocks){
	if (ino != 0) {
		fprintf(stderr, "!!raid4disk_setsize: ino != 0 not supported\n");
		return -1;
	}

	struct raid4disk_state *rds = bi->state;
    for (unsigned int i = 0; i < rds->nbelow; i++) {
        (void) (*rds->below[i]->setsize)(rds->below[i], 0, nblocks);
    }
    return 0;
}

static int raid4disk_read(block_if bi, unsigned int ino, block_no offset, block_t *block){
	if (ino != 0) {
		fprintf(stderr, "!!raid4disk_read: ino != 0 not supported\n");
		return -1;
	}

    // Your code goes here
    return 0;
}

static int raid4disk_write(block_if bi, unsigned int ino, block_no offset, block_t *block){
	struct raid4disk_state *rds = bi->state;

	if (ino != 0) {
		fprintf(stderr, "!!raid4disk_write: ino != 0 not supported\n");
		return -1;
	}

    // Your code goes here
    return -1;
}

static void raid4disk_release(block_if bi){
	free(bi->state);
	free(bi);
}

static int raid4disk_sync(block_if bi, unsigned int ino){
	/* Sync all block stores below.
	 */
	struct raid4disk_state *rds = bi->state;
	for (unsigned int i = 0; i < rds->nbelow; i++) {
		if ((*rds->below[i]->sync)(rds->below[i], ino) < 0) {
			fprintf(stderr, "!!raid4disk_sync: sync error for block store %d below\n", i);
			return -1;
		}
	}
	return 0;
}

block_if raid4disk_init(block_if *below, unsigned int nbelow){
    if (nbelow <= 1) {
		fprintf(stderr, "!!raid4disk_init: need at least 2 disks\n");
        return NULL;
    }

	/* Create the block store state structure.
	 */
	struct raid4disk_state *rds = new_alloc(struct raid4disk_state);
	rds->below = below;
	rds->nbelow = nbelow;

	/* Return a block interface to this inode.
	 */
	block_if bi = new_alloc(block_store_t);
	bi->state = rds;
	bi->getninodes = raid4disk_getninodes;
	bi->getsize = raid4disk_getsize;
	bi->setsize = raid4disk_setsize;
	bi->read = raid4disk_read;
	bi->write = raid4disk_write;
	bi->release = raid4disk_release;
	bi->sync = raid4disk_sync;
	return bi;
}
