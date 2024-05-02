#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <mach-o/loader.h>
#include "../h/earth/earth.h"
#include "../h/egos/exec.h"

static void usage(char *name){
	fprintf(stderr, "Usage: %s macho-executable egos-executable\n", name);
	exit(1);
}

/* Parse a Mach-O format file.
 */
int main(int argc, char **argv){
	if (argc != 3) {
		usage(argv[0]);
	}

	/* Open the two files.
	 */
	FILE *input, *output;
	if ((input = fopen(argv[1], "r")) == 0) {
		fprintf(stderr, "%s: can't open macho executable %s\n",
									argv[0], argv[1]);
		return 1;
	}
	if ((output = fopen(argv[2], "w")) == 0) {
		fprintf(stderr, "%s: can't create egos executable %s\n",
									argv[0], argv[2]);
		return 1;
	}

	union {
		struct load_command lc;
		struct segment_command_64 sc;
	} u;
	struct section_64 se;

	/* Read the macho header.
	 */
	struct mach_header_64 hdr;
	int n = fread((char *) &hdr, sizeof(hdr), 1, input);
	if (n != 1) {
		fprintf(stderr, "%s: can't read header in %s\n", argv[0], argv[1]);
		return 1;
	}
	if (hdr.magic != MH_MAGIC_64 /* || hdr.filetype != MH_PRELOAD */) {
		fprintf(stderr, "%s: %s not a 64bit executable\n", argv[0], argv[1]);
		return 1;
	}

	/* Output header.
	 */
	struct exec_header eh;
	eh.eh_nsegments = 0;
	eh.eh_offset = 1;

    off_t offset = PAGESIZE;
    address_t start_addr = 0;

	/* Read the "commands".
	 */
	fpos_t inpos = sizeof(hdr), outpos = sizeof(eh);
	unsigned int i, j;
	for (i = 0; i < hdr.ncmds; i++) {
		/* Seek to the next command and read the command header.
		 */
		fseek(input, inpos, SEEK_SET);
		n = fread((char *) &u.lc, sizeof(u.lc), 1, input);
		if (n != 1) {
			fprintf(stderr, "%s: bad command in %s\n", argv[0], argv[1]);
			return 1;
		}
		inpos += sizeof(u.lc);

		/* TODO.  Extract start address and stack size??
		 */
		if (u.lc.cmd == LC_MAIN) {
			fprintf(stderr, "LC_MAIN\n");
			continue;
		}

		/* Skip other stuff except segments.
		 */
		if (u.lc.cmd != LC_SEGMENT_64) {
			inpos += u.lc.cmdsize - sizeof(u.lc);
			continue;
		}

		/* Read the remainder of the command.
		 */
		n = fread((char *) &u.sc + sizeof(u.lc),
									sizeof(u.sc) - sizeof(u.lc), 1, input);
		if (n != 1) {
			fprintf(stderr, "%s: bad segment in %s\n", argv[0], argv[1]);
			return 1;
		}
		inpos += sizeof(u.sc) - sizeof(u.lc);
		if (u.sc.nsects == 0) {
			continue;
		}

		printf("%s: vmaddr=%llx vmsize=%llx fileoff=%llx filesz=%llx prot=%x outpos=%d\n", argv[1], u.sc.vmaddr, u.sc.vmsize, u.sc.fileoff, u.sc.filesize, u.sc.initprot, (int) outpos);

		/* Some sanity checks.
		 */
		if (u.sc.vmaddr % PAGESIZE != 0) {
			fprintf(stderr, "%s: segment offset not page aligned\n", argv[0]);
			return 1;
		}
		if (u.sc.vmsize % PAGESIZE != 0) {
			fprintf(stderr, "%s: segment size not multiple of pages\n", argv[0]);
			return 1;
		}
		if (u.sc.fileoff % PAGESIZE != 0) {
			fprintf(stderr, "%s: file offset not page aligned\n", argv[0]);
			return 1;
		}
		if (u.sc.filesize % PAGESIZE != 0) {
			fprintf(stderr, "%s: file size not page aligned\n", argv[0]);
			return 1;
		}

		/* Create a segment descriptor.
		 */
		struct exec_segment es;
		es.es_first = u.sc.vmaddr / PAGESIZE;
		es.es_npages = u.sc.vmsize / PAGESIZE,
		es.es_nblocks = u.sc.filesize / PAGESIZE,
		es.es_prot = 0;
		if (u.sc.initprot & PROT_READ) {
			es.es_prot |= P_READ;
		}
		if (u.sc.initprot & PROT_WRITE) {
			es.es_prot |= P_WRITE;
		}
		if (u.sc.initprot & PROT_EXEC) {
			es.es_prot |= P_EXEC;
            if (start_addr == 0) {
                start_addr = u.sc.vmaddr;
            }
		}

		/* Write the segment descriptor to the output file.
		 */
		fseek(output, outpos, SEEK_SET);
		fwrite((char *) &es, sizeof(es), 1, output);
		outpos += sizeof(es);
		eh.eh_nsegments++;

		/* Copy the segment into the output file.
		 */
		fseek(input, u.sc.fileoff, SEEK_SET);
		fseek(output, offset, SEEK_SET);
		char buf[PAGESIZE];
		unsigned int total = u.sc.filesize, size;
		while (total > 0) {
			size = total > PAGESIZE ? PAGESIZE : total;
			if ((n = fread(buf, 1, size, input)) <= 0) {
				fprintf(stderr, "%s: unexpexted EOF in %s\n", argv[0], argv[1]);
				return 1;
			}
			fwrite(buf, 1, n, output);
			total -= n;
		}

		/* See if the start address is in one of the sections.
		 */
		fseek(input, inpos, SEEK_SET);
		for (j = 0; j < u.sc.nsects; j++) {
			n = fread((char *) &se, sizeof(se), 1, input);
			if (n != 1) {
				fprintf(stderr, "%s: bad section in %s\n", argv[0], argv[1]);
				return 1;
			}
			inpos += sizeof(se);
			if (strcmp(se.sectname, "__text") == 0) {
				eh.eh_start = se.addr;
			}
		}

        offset += u.sc.filesize;
	}

    if (eh.eh_start == 0) {
        eh.eh_start = start_addr;
    }

	/* Write the output header.
	 */
	fseek(output, 0, SEEK_SET);
	fwrite((char *) &eh, sizeof(eh), 1, output);
	fclose(output);
	fclose(input);

	return 0;
}
