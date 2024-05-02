/* Author: Robbert van Renesse, August 2015
 *
 * This block store module mirrors the underlying block store but contains
 * a write-through cache.  The caching strategy is CLOCK, approximating LRU.
 * The interface is as follows:
 *
 *		block_if clockdisk_init(block_if below,
 *									block_t *blocks, block_no nblocks)
 *			'below' is the underlying block store.  'blocks' points to
 *			a chunk of memory wth 'nblocks' blocks for caching.
 *
 *		void clockdisk_dump_stats(block_if bi)
 *			Prints the cache statistics.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <egos/block_store.h>

/* State contains the pointer to the block module below as well as caching
 * information and caching statistics.
 */
struct clockdisk_state
{
	block_if below;		// block store below
	block_t *blocks;	// memory for caching blocks
	block_no nblocks; // size of cache (not size of block store!)

	/* Stats.
	 */
	unsigned int read_hit, read_miss, write_hit, write_miss;
};
typedef struct clock_info
{
	int use;
	int inode;
	int offset;
	int dirty;
} clock_info;
typedef struct cache
{
	clock_info *metadata;
	int nodes;
} cache;
static cache clock_cache;
// #define clock_info *m clock_cache.metadata;
// #define n clock_cache.nodes;
static int clock_init(int k)
{
	// cache *clock = (cache *)malloc(sizeof(cache));
	// if (clock == NULL)
	// {
	// 	return -1;
	// }
	printf("wb: before clock_init mallocs");
	clock_info *metadata = (clock_info *)malloc(sizeof(clock_info));
	printf("wb: after clock_init mallocs");
	if (metadata == NULL)
	{
		return -1;
	}
	for (int i = 0; i <= k; i++)
	{
		metadata[i].use, metadata[i].inode, metadata[i].offset, metadata[i].dirty = -1;
	}
	clock_cache.metadata = metadata;
	clock_cache.nodes = k;
	printf("wb:clock_init finishes");
	return 0;
}
int cache_update(block_if bi, unsigned int ino, block_no offset, block_t *block)
// current issue: how do i keep track of when dirty goes from 0->1
{
	printf("wb: cache_update called");
	struct clockdisk_state *cs = bi->state;
	while (1)
	{
		int used_ctr = 0;
		int i = 0;
		if (clock_cache.metadata[i].use == -1)
		{
			clock_cache.metadata[i].use = 1;
			clock_cache.metadata[i].inode = ino;
			clock_cache.metadata[i].offset = offset;
			clock_cache.metadata[i].dirty = 1;
			printf("wb: cache_update finished");
			return 0; // success
		}
		if (clock_cache.metadata[i].use == 0)
		{
			if (clock_cache.metadata[i].dirty == 1)
			{
				// HUHHHHH????
				printf("wb: cache_update finished");
				return (*cs->below->write)(cs->below, ino, offset, block);
				// memcpy(cs->blocks[i].bytes, block->bytes, BLOCK_SIZE)
			}
			clock_cache.metadata[i].use = 1;
			clock_cache.metadata[i].inode = ino;
			clock_cache.metadata[i].offset = offset;
			printf("wb: cache_update finished");
			return 0; // success!
		}
		else if (clock_cache.metadata[i].use == 1)
		{
			clock_cache.metadata[i].use = 1; // steps to success
		}
		// modular addition, no?
		i = (i++) % clock_cache.nodes;
	}
	
}

static int clockdisk_getninodes(block_store_t *this_bs)
{
	struct clockdisk_state *cs = this_bs->state;
	return (*cs->below->getninodes)(cs->below);
}

static int clockdisk_getsize(block_if bi, unsigned int ino)
{
	struct clockdisk_state *cs = bi->state;
	return (*cs->below->getsize)(cs->below, ino);
}

static int clockdisk_setsize(block_if bi, unsigned int ino, block_no nblocks)
{
	struct clockdisk_state *cs = bi->state;

	return (*cs->below->setsize)(cs->below, ino, nblocks);
}

static int clockdisk_read(block_if bi, unsigned int ino, block_no offset, block_t *block)
{
	/* Your code should replace this naive implementation
	 */
	  printf("wb: read started");
	struct clockdisk_state *cs = bi->state;
	for (int i = 0; i < clock_cache.nodes; i++)
	{
		if (clock_cache.metadata[i].inode == ino && clock_cache.metadata[i].offset == offset)
		{
			memcpy(block->bytes, cs->blocks[i].bytes, BLOCK_SIZE);
			printf("wb: read finished");
			return 0;
		}
	}
	int r = (*cs->below->read)(cs->below, ino, offset, block);
	cache_update(bi, ino, offset, block);
	return r;
	printf("wb: read finished");
}

static int clockdisk_write(block_if bi, unsigned int ino, block_no offset, block_t *block)
{
	/* Your code should replace this naive implementation
	 */
	printf("wb: write started");
	struct clockdisk_state *cs = bi->state;
	// block_t temp = *block;
	for (int i = 0; i < clock_cache.nodes; i++)
	{
		if (clock_cache.metadata[i].inode == ino && clock_cache.metadata[i].offset == offset)
		{
			memcpy(cs->blocks[i].bytes, block->bytes, BLOCK_SIZE);
			printf("wb: write finished");
			return 0;
		}
		printf("wb: write finished");
		return cache_update(bi, ino, offset, block);
	}
	printf("wb: write finished");
	// cache_update(bi, ino, offset, block);
	// return (*cs->below->write)(cs->below, ino, offset, block);
}

static void wtclockdisk_release(block_if bi)
{
	struct wtclockdisk_state *cs = bi->state;
	free(cs);
	free(bi);
}

static int clockdisk_sync(block_if bi, unsigned int ino)
{
	/* Your code goes here:
	 */
	 printf("wb: sync started");
	struct clockdisk_state *cs = bi->state;
	if (ino == -1)
	{
		for (int i = 0; i < clock_cache.nodes; i++)
		{
			if (clock_cache.metadata[i].dirty == 1)
			{
				(*cs->below->sync)(bi, ino);
				clock_cache.metadata[i].dirty = 0;
			}
		}
	}
	else
	{
		for (int i = 0; i < clock_cache.nodes; i++)
		{
			if (clock_cache.metadata[i].inode == ino)
			{
				if (clock_cache.metadata[i].dirty == 1)
				{
					// HOW AM I SUPPOSED TO GET THE DATA IF I DONT HAVE BLOCK??
					// WHAT DOES INO

					clock_cache.metadata[i].dirty = 1;
					(*cs->below->sync)(bi, ino);
					printf("wb: sync finished");
					return 0;
				}
				printf("wb: sync finished");
				return 1;
			}
		}
	}
	printf("wb: sync finished");
	return 1;
}

static void clockdisk_release(block_if bi)
{
	struct clockdisk_state *cs = bi->state;
	free(cs);
	free(bi);
}

void clockdisk_dump_stats(block_if bi)
{
	struct clockdisk_state *cs = bi->state;

	printf("!$CLOCK: #read hits:    %u\n", cs->read_hit);
	printf("!$CLOCK: #read misses:  %u\n", cs->read_miss);
	printf("!$CLOCK: #write hits:   %u\n", cs->write_hit);
	printf("!$CLOCK: #write misses: %u\n", cs->write_miss);
}

/* Create a new block store module on top of the specified module below.
 * blocks points to a chunk of memory of nblocks blocks that can be used
 * for caching.
 */
block_if clockdisk_init(block_if below, block_t *blocks, block_no nblocks)
{
	/* Create the block store state structure.
	 */
	struct clockdisk_state *cs = new_alloc(struct clockdisk_state);
	cs->below = below;
	cs->blocks = blocks;
	cs->nblocks = nblocks;
	cs->read_hit = 0;
	cs->read_miss = 0;
	cs->write_hit = 0;
	cs->write_miss = 0;

	/* Return a block interface to this inode.
	 */
	block_if bi = new_alloc(block_store_t);
	bi->state = cs;
	bi->getninodes = clockdisk_getninodes;
	bi->getsize = clockdisk_getsize;
	bi->setsize = clockdisk_setsize;
	bi->read = clockdisk_read;
	bi->write = clockdisk_write;
	bi->release = clockdisk_release;
	bi->sync = clockdisk_sync;
	return bi;
}
