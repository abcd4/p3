#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <assert.h>
// #include <sys/mman.h>
#include <elf.h>
#include "../h/earth/earth.h"
#include "../h/egos/exec.h"

#define VERBOSE

#define MAX_SEGMENTS 4

struct page_info {
	unsigned int prot;		// protection bits
	int initialized;		// initialized or not
};

static void usage(char *name){
	fprintf(stderr, "Usage: %s elf-executable egos-executable\n", name);
	exit(1);
}

void image_grow(char **image, struct page_info **pages, unsigned int *npages,
						unsigned long newsize){
	if (newsize > (unsigned long) *npages * PAGESIZE) {
		int before = *npages;
		*npages = (newsize + PAGESIZE - 1) / PAGESIZE;
		*image = realloc(*image, *npages * PAGESIZE);
		memset(&(*image)[before * PAGESIZE], 0, (*npages - before) * PAGESIZE);
		*pages = realloc(*pages, *npages * sizeof(**pages));
		memset(&(*pages)[before], 0, (*npages - before) * sizeof(**pages));
	}
}

void image_prot(struct page_info *pages, unsigned int npages,
					unsigned int offset, unsigned int size,
					unsigned int flags, int initialized) {
	for (unsigned int addr = offset; addr < offset + size; addr += PAGESIZE) {
		unsigned int page = addr / PAGESIZE;
		assert(page < npages);

		if (flags & PF_R) {
			pages[page].prot |= P_READ;
		}
		if (flags & PF_W) {
			pages[page].prot |= P_WRITE;
		}
		if (flags & PF_X) {
			pages[page].prot |= P_EXEC;
		}
		pages[page].initialized |= initialized;
	}
}

/* Parse an ELF format file.
 */
int main(int argc, char **argv){
	if (argc != 3) {
		usage(argv[0]);
	}

	/* Open the two files.
	 */
	FILE *input, *output;
	if ((input = fopen(argv[1], "r")) == 0) {
		fprintf(stderr, "%s: can't open elf executable %s\n",
									argv[0], argv[1]);
		return 1;
	}
	if ((output = fopen(argv[2], "w")) == 0) {
		fprintf(stderr, "%s: can't create grass executable %s\n",
									argv[0], argv[2]);
		return 1;
	}

#ifdef x86_32
	Elf32_Ehdr ehdr;
#endif
#ifdef x86_64
	Elf64_Ehdr ehdr;
#endif

	size_t n = fread((char *) &ehdr, sizeof(ehdr), 1, input);
	if (n != 1) {
		fprintf(stderr, "%s: can't read header in %s (%d)\n", argv[0], argv[1], (int) n);
		return 1;
	}
	if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
			ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
			ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
			ehdr.e_ident[EI_MAG3] != ELFMAG3 ||
#ifdef x86_32
			ehdr.e_ident[EI_CLASS] != ELFCLASS32 ||
#endif
#ifdef x86_64
			ehdr.e_ident[EI_CLASS] != ELFCLASS64 ||
#endif
			ehdr.e_type != ET_EXEC) {
		fprintf(stderr, "%s: %s not an executable\n", argv[0], argv[1]);
		return 1;
	}

	/* Output header.
	 */
	struct exec_header eh;
	memset(&eh, 0, sizeof(eh));
	eh.eh_offset = 1;
	eh.eh_start = ehdr.e_entry;

	/* Read the section table.
	 */
#ifdef x86_32
	Elf32_Shdr *shdr = calloc(sizeof(*shdr), ehdr.e_shnum);
#endif
#ifdef x86_64
	Elf64_Shdr *shdr = calloc(sizeof(*shdr), ehdr.e_shnum);
#endif
	fseek(input, ehdr.e_shoff, SEEK_SET);
	n = fread((char *) shdr, sizeof(*shdr), ehdr.e_shnum, input);
	if (n != ehdr.e_shnum) {
		fprintf(stderr, "%s: can't read section headers in %s (%d %d)\n",
						argv[0], argv[1], (int) n, (int) ehdr.e_shnum);
		return 1;
	}
	int rela_index = -1, dynsym_index = -1;
	for (int i = 0; i < ehdr.e_shnum; i++) {
		switch (shdr[i].sh_type) {
		case SHT_RELA:
			rela_index = i;
			break;
		case SHT_DYNSYM:
			dynsym_index = i;
			break;
		}
	}
#ifdef VERBOSE
	if (rela_index > 0) {
		printf("rela %d %lx %lx\n", shdr[rela_index].sh_type, shdr[rela_index].sh_addr, (unsigned long) shdr[rela_index].sh_offset);
	}
	if (dynsym_index > 0) {
		printf("dynsym %d %lx %lx\n", shdr[dynsym_index].sh_type, shdr[dynsym_index].sh_addr, (unsigned long) shdr[dynsym_index].sh_offset);
	}
#endif

	/* Read the dynsym table.
	 */
#ifdef x86_32
	Elf32 *stab = 0;
#endif
#ifdef x86_64
	Elf64_Sym *stab = 0;
#endif
	if (dynsym_index > 0) {
		stab = malloc(shdr[dynsym_index].sh_size);
		fseek(input, shdr[dynsym_index].sh_offset, SEEK_SET);
		n = fread((char *) stab, 1, shdr[dynsym_index].sh_size, input);
		if (n != shdr[dynsym_index].sh_size) {
			fprintf(stderr, "can't read dynsym table\n");
			return 1;
		}
#ifdef VERBOSE
		unsigned int dynsym_cnt = shdr[dynsym_index].sh_size / shdr[dynsym_index].sh_entsize;
		printf("#entries in dynsym: %u\n", dynsym_cnt);
#endif
	}

#ifdef x86_32
	Elf32_Phdr phdr;
#endif
#ifdef x86_64
	Elf64_Phdr phdr;
#endif

	/* Create the image and keep track of page info.
	 */
	unsigned int inpos = ehdr.e_phoff;	// position in input file
	address_t base = 0;					// virtual base address of image
	char *image = NULL;					// contents of image
	struct page_info *pages = NULL;		// info per page
	unsigned int npages = 0;			// number of pages
	for (int i = 0; i < ehdr.e_phnum; i++) {
		fseek(input, inpos, SEEK_SET);
		n = fread((char *) &phdr, sizeof(phdr), 1, input);
		if (n != 1) {
			fprintf(stderr, "%s: can't read program header in %s\n",
												argv[0], argv[1]);
			return 1;
		}
		inpos += sizeof(phdr);

		if (phdr.p_memsz == 0 || phdr.p_type == PT_INTERP || phdr.p_type == PT_DYNAMIC) {
			continue;
		}

		printf("%s: %d type=%x vaddr=%lx-%lx memsz=%lx offset=%lx-%lx filesz=%lx prot=%x\n", argv[1], i, phdr.p_type, phdr.p_vaddr, phdr.p_vaddr + phdr.p_memsz, phdr.p_memsz, phdr.p_offset, phdr.p_offset + phdr.p_filesz, phdr.p_filesz, phdr.p_flags);

		if (base == 0) {
			base = phdr.p_vaddr;
		}
		else if (phdr.p_vaddr < base) {
			continue;
		}
		assert(phdr.p_memsz >= phdr.p_filesz);

		/* See if we need to grow the image.
		 */
		image_grow(&image, &pages, &npages, phdr.p_vaddr + phdr.p_memsz - base);

		/* Copy the data into the right place into the image.
		 */
		fseek(input, phdr.p_offset, SEEK_SET);
		unsigned int total = phdr.p_filesz;
		char *dst = &image[phdr.p_vaddr - base];
		while (total > 0) {
			if ((n = fread(dst, 1, total, input)) <= 0) {
				fprintf(stderr, "%s: unexpected EOF in %s\n", argv[0], argv[1]);
				return 1;
			}
			total -= n;
			dst += n;
		}

		/* Set the protection bits on the pages.
		 */
		image_prot(pages, npages, phdr.p_vaddr - base,
							phdr.p_filesz, phdr.p_flags, 1);
		image_prot(pages, npages, phdr.p_vaddr + phdr.p_filesz - base,
							phdr.p_memsz - phdr.p_filesz, phdr.p_flags, 0);
	}

	/* Read the GOT.
	 */
	if (rela_index > 0 && stab != 0) {
		fseek(input, shdr[rela_index].sh_offset, SEEK_SET);
		for (size_t i = 0; i < shdr[rela_index].sh_size / shdr[rela_index].sh_entsize; i++) {
#ifdef x86_64
			Elf64_Rela er;
			if (fread(&er, 1, shdr[rela_index].sh_entsize, input) != shdr[rela_index].sh_entsize) {
				fprintf(stderr, "%s: can't read rela in %s\n", argv[0], argv[1]);
				return 1;
			}
			switch (ELF64_R_TYPE(er.r_info)) {
			case R_X86_64_GLOB_DAT:
#ifdef VERBOSE
				printf("glob offset=%lx addr=%lx size=%d\n", er.r_offset, stab[ELF64_R_SYM(er.r_info)].st_value, (int) sizeof(stab[ELF64_R_SYM(er.r_info)].st_value));
#endif
				image_grow(&image, &pages, &npages, er.r_offset + sizeof(stab[ELF64_R_SYM(er.r_info)].st_value) - base);
				image_prot(pages, npages, er.r_offset - base, sizeof(stab[ELF64_R_SYM(er.r_info)].st_value), PF_R, 1);
				memcpy(&image[er.r_offset - base],
					&stab[ELF64_R_SYM(er.r_info)].st_value,
					sizeof(stab[ELF64_R_SYM(er.r_info)].st_value));
				break;
			case R_X86_64_RELATIVE:
#ifdef VERBOSE
				printf("rela offset=%lx addend=%lx size=%d\n", er.r_offset, er.r_addend, (int) sizeof(er.r_addend));
#endif
				image_grow(&image, &pages, &npages, er.r_offset + sizeof(er.r_addend) - base);
				image_prot(pages, npages, er.r_offset - base, sizeof(er.r_addend), PF_R, 1);
				memcpy(&image[er.r_offset - base],
					&er.r_addend, sizeof(er.r_addend));
				break;
			}
#endif
		}
	}

	/* Split the pages up into segments.  Each segment consists of a
	 * sequence of pages of the same protection flags.  Moreover, it
	 * can consist of some initialized pages followed by some uninitialized
	 * ones.
	 */
	struct exec_segment es[MAX_SEGMENTS];
	memset(es, 0, sizeof(es));
	unsigned int first_page = 0;
	unsigned int last_initialized = pages[0].initialized;
	for (unsigned int i = 1; i <= npages; i++) {
		if (i == npages || pages[i].prot != pages[first_page].prot ||
				(pages[i].initialized && !last_initialized)) {
			if (pages[first_page].prot != 0) {
				unsigned int ninitialized = 0;
				for (unsigned int j = first_page; j < i; j++) {
					if (pages[j].initialized) {
						ninitialized++;
					}
					else {
						break;
					}
				}

				assert(eh.eh_nsegments < MAX_SEGMENTS - 1);
				es[eh.eh_nsegments].es_first = first_page + (base / PAGESIZE);
				es[eh.eh_nsegments].es_npages = i - first_page;
				es[eh.eh_nsegments].es_nblocks = ninitialized;
				es[eh.eh_nsegments].es_prot = pages[first_page].prot;
				eh.eh_nsegments++;
			}
			first_page = i;
		}
		if (i < npages) {
			last_initialized = pages[i].initialized;
		}
	}

	/* Write the output header.
	 */
	fwrite((char *) &eh, sizeof(eh), 1, output);
	fwrite((char *) es, sizeof(es), 1, output);

	/* Now write the segments.
	 */
    fseek(output, PAGESIZE, SEEK_SET);
	for (unsigned int i = 0; i < eh.eh_nsegments; i++) {
		unsigned int addr = (es[i].es_first * PAGESIZE) - base;
		size_t n = fwrite(&image[addr], PAGESIZE, es[i].es_nblocks, output);
		assert(n == es[i].es_nblocks);
	}

	fclose(output);
	fclose(input);
	printf("START %lx\n", (unsigned long) eh.eh_start);

	return 0;
}
