#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <block_store.h>

#define DISK_SIZE 15
#define MAX_INODES 6
#define NDISKS  3

struct disk {
    block_t blocks[DISK_SIZE];
};

static struct disk disks[NDISKS]; // blocks for ram_disk

void dump_blocks(){
    for (unsigned int i = 0; i < DISK_SIZE; i++) {
        printf("%2d", i);
        for (unsigned int disk = 0; disk < NDISKS; disk++) {
            printf(" |");
            block_no *b = (block_no *) &disks[disk].blocks[i];
            for (int j = 0; j < WORDS_PER_BLOCK; j++) {
                printf(" %2d", *b++);
            }
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    assert(sizeof(block_no) == 4);

    block_store_t *ramdisks[NDISKS];
    for (unsigned int i = 0; i < NDISKS; i++) {
        ramdisks[i] = ramdisk_init(disks[i].blocks, DISK_SIZE);
    }
    block_store_t *fdisk = raid4disk_init(ramdisks, NDISKS);
    block_store_t *cdisk = checkdisk_init(fdisk, "raid", NULL);

    for (int cnt = 1;; cnt++) {
        dump_blocks();

        char line[128];
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        char cmd;
        unsigned int inode, bno, data;
        unsigned int nargs = sscanf(line, "%c:%u:%u:%u", &cmd, &inode, &bno, &data);
        if (nargs == 0 || cmd == '\n') {
            continue;
        }
        if (cmd == 'Q') {
            break;
        }
        static block_t block;
        int result;
        switch (cmd) {
        case 'R':
            assert(nargs == 3 || nargs == 4);
            result = (*cdisk->read)(cdisk, inode, bno, &block);
            if (result < 0) {
                fprintf(stderr, "!!ERROR: tracedisk_run: read(%u, %u) failed\n", inode, bno);
                break;
            }
            if (nargs == 3 &&
                (
                 (((unsigned int *) &block)[0] != inode && ((unsigned int *) &block)[0] != 0)
                 || (((unsigned int *) &block)[1] != bno && ((unsigned int *) &block)[1] != 0)
                 ))
                {
                    fprintf(stderr, "!!ERROR: tracedisk_run: unexpected content %u %u %u %u\n", inode, bno, ((unsigned int *) &block)[0], ((unsigned int *) &block)[1]);
                }
            if ((nargs == 4) && (((unsigned int *) &block)[2] != data)) {
                fprintf(stderr, "!!ERROR @line%u: tracedisk_run: read %u %u: %u != %u\n",
                        cnt, inode, bno, data, ((unsigned int *) &block)[2]);
            }
            break;
        case 'W':
            assert(nargs == 3 || nargs == 4);
            ((unsigned int *) &block)[0] = inode;
            ((unsigned int *) &block)[1] = bno;
            if (nargs == 4) {
                for (int j = 0; j < WORDS_PER_BLOCK; j++) {
                    ((unsigned int *) &block)[j] = data;
                }
            } else {
                for (int j = 0; j < WORDS_PER_BLOCK; j++) {
                    ((unsigned int *) &block)[j] = cnt; // line# by default
                }
            }
            result = (*cdisk->write)(cdisk, inode, bno, &block);
            if (result < 0) {
                fprintf(stderr, "!!ERROR: tracedisk_run: write(%u, %u) failed\n", inode, bno);
                break;
            }
            break;
        case 'S':
            assert(nargs == 3);
            result = (*cdisk->setsize)(cdisk, inode, bno);
            if (result < 0) {
                fprintf(stderr, "!!ERROR: tracedisk_run: setsize(%u, %u) failed\n", inode, bno);
                break;
            }
            break;
        case 'G':
            assert(nargs == 2 || nargs == 3);
            result = (*cdisk->getsize)(cdisk, inode);
            if (result < 0) {
                fprintf(stderr, "!!ERROR: tracedisk_run: getsize(%u, %u) failed\n", inode, bno);
                break;
            }
            if (nargs == 2) {
                printf("getsize --> %u\n", result);
            }
            else if ((nargs == 3) && (result != bno)) {
                fprintf(stderr, "!!ERROR @line%u: tracedisk_run: getsize %u: %u != %u\n",
                        cnt, inode, bno, result);
            }
            break;
        case 'F':
            assert(nargs == 2);
            result = (*cdisk->sync)(cdisk, inode);
            if (result < 0) {
                fprintf(stderr, "!!ERROR: tracedisk_run: sync(%u) failed\n", inode);
            }
            break;
        case 'K':
            assert(nargs == 2);
            ramdisk_set_fail(ramdisks[inode], 1);
            break;
        default:
            printf("unknown command\n");
        }
    }

    (*cdisk->release)(cdisk);
    (*fdisk->release)(fdisk);
    for (unsigned int i = 0; i < NDISKS; i++) {
        (*ramdisks[i]->release)(ramdisks[i]);
    }

    return 0;
}
