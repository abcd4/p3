#ifndef BLOCK_STORE_H
#define BLOCK_STORE_H

#define new_alloc(t)    ((t *) malloc(sizeof(t)))

typedef unsigned int block_no;    // index of a block
#define WORDS_PER_BLOCK    4
#define BLOCK_SIZE         (WORDS_PER_BLOCK * sizeof(block_no))

typedef struct block {
    unsigned char bytes[BLOCK_SIZE];
} block_t;

typedef struct block_store {
    void *state;
    int (*read)(struct block_store *this_bs, unsigned int ino, block_no offset, block_t *block);
    int (*write)(struct block_store *this_bs, unsigned int ino, block_no offset, block_t *block);
    int (*getsize)(struct block_store *this_bs, unsigned int ino);
    int (*setsize)(struct block_store *this_bs, unsigned int ino, block_no newsize);

    void (*release)(struct block_store *this_bs);
    int (*getninodes)(struct block_store *this_bs);
    int (*sync)(struct block_store *this_bs, unsigned int ino);
} block_store_t;

typedef block_store_t *block_if;  // block store interface

/* Each block store module has an 'init' function that returns a
 * 'block_store_t *' type.  Here are the 'init' functions of various
 * available block store types.
 */
block_if checkdisk_init(block_store_t *below, const char *descr, unsigned int *report);
block_if raid4disk_init(block_if *below, unsigned int nbelow);
block_if ramdisk_init(block_t *blocks, block_no nblocks);
/* block_if clockdisk_init(block_if below, block_t *blocks, block_no nblocks); */
/* block_if combinedisk_init(block_if *below, unsigned int nbelow); */
/* block_if debugdisk_init(block_if below, const char *descr); */
/* block_if fatdisk_init(block_if below, unsigned int below_ino); */
/* block_if filedisk_init(const char *file_name, block_no nblocks); */
/* block_if mapdisk_init(block_if below, unsigned int ino); */
/* block_if partdisk_init(block_if below, unsigned int ninodes, block_no partsizes[]); */
/* block_if raid0disk_init(block_if *below, unsigned int nbelow); */
/* block_if raid1disk_init(block_if *below, unsigned int nbelow); */
/* block_if raid5disk_init(block_if *below, unsigned int nbelow); */
/* block_if statdisk_init(block_if below); */
/* block_if tracedisk_init(block_if below, char *trace); */
/* block_if treedisk_init(block_if below, unsigned int below_ino); */
/* block_if unixdisk_init(block_if below, unsigned int below_ino); */
/* block_if wtclockdisk_init(block_if below, block_t *blocks, block_no nblocks); */

/* Some useful functions on some block store types.
 */
int ramdisk_set_fail(block_store_t *this_bs, int failed);
/* int treedisk_create(block_if below, unsigned int below_ino, unsigned int ninodes); */
/* int fatdisk_create(block_if below, unsigned int below_ino, unsigned int ninodes); */
/* int unixdisk_create(block_if below, unsigned int below_ino, unsigned int ninodes); */

/* int treedisk_check(block_if below); */
/* void clockdisk_dump_stats(block_if this_bs); */
/* void statdisk_dump_stats(block_if this_bs); */

#endif
