#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <earth/earth.h>
#include <earth/intf.h>
#include <egos/exec.h>
#include <egos/context.h>

/* The structure that contains all interfaces the kernel can use
 * to communicate with the Earth virtual machine monitor.
 */
struct earth_intf earth;

void earth_setup(){
	log_setup(&earth.log);
	intr_setup(&earth.intr);
	tlb_setup(&earth.tlb);
	clock_setup(&earth.clock);
	dev_disk_setup(&earth.dev_disk);
	dev_gate_setup(&earth.dev_gate);
	dev_tty_setup(&earth.dev_tty);
	dev_udp_setup(&earth.dev_udp);
}

static struct exec_header eh;		// exec header of the kernel

void ctx_entry(void){
	(*((void (*)()) eh.eh_start))();
}

int main(){
	earth_setup();

	/* Create the mapped region that will hold the kernel.
	 */
	char *b = mmap((void *) KERN_BASE, KERN_PAGES * PAGESIZE,
						PROT_READ | PROT_WRITE,
						MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
	assert(b == (char *) KERN_BASE);

	printf("earth: loading the O.S. kernel\n\r");

	/* Load the kernel.
	 */
	int fd = open("build/grass/k.exe", O_RDONLY);
	assert(fd > 0);
	ssize_t n;
	n = read(fd, &eh, sizeof(eh));
	assert(n == sizeof(eh));
	assert(eh.eh_offset == 1);

    /* Read the segments, one at a time.
     */
    int pos = sizeof(eh);           // position of segment header in file
    off_t offset = eh.eh_offset * PAGESIZE;    // page offset in file
    struct exec_segment es;
    for (unsigned int i = 0; i < eh.eh_nsegments; i++) {
        lseek(fd, pos, SEEK_SET);
        n = read(fd, &es, sizeof(es));
        assert(n == sizeof(es));
        lseek(fd, offset, SEEK_SET);
        size_t size = (size_t) es.es_nblocks * PAGESIZE;
        // printf("LOAD %lx %lx\n\r", ((unsigned long) es.es_first * PAGESIZE), size);
        b = (char *) ((off_t) es.es_first * PAGESIZE);
        while (size > 0) {
            n = read(fd, b, size);
            assert(n >= 0);
            if (n == 0) {
                break;
            }
            b += n;
            size -= n;
            offset += n;
        }

        // Set the protection correctly
        b = (char *) ((off_t) es.es_first * PAGESIZE);
        size = (size_t) es.es_npages * PAGESIZE;
        if (mprotect(b, size, prot_cvt(es.es_prot)) != 0) {
            perror("earthbox: mprotect");
        }

        pos += sizeof(es);
    }

	/* Put the ``earth'' interface in the top of the kernel's address
	 * space.  Through this interface the kernel can communicate with
	 * this virtual machine monitor.
	 */
	memcpy((void *) (KERN_TOP - sizeof(earth)), &earth, sizeof(earth));

	printf("earth: booting O.S. kernel at 0x%lx\n\r", (unsigned long) eh.eh_start);
	address_t old_sp;
	ctx_start(&old_sp, KERN_TOP - PAGESIZE);
	assert(0);

	return 0;
}
