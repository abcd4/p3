#ifndef EGOS_EXEC_H
#define EGOS_EXEC_H

/* Header of an executable.  Followed by a list of segments.
 */
struct exec_header {
	unsigned int eh_nsegments;	// #segments
	unsigned int eh_offset;		// segments offset in file (in pages)
	address_t eh_start;			// initial instruction pointer
};

/* Segments in an executable.
 */
struct exec_segment {
	unsigned int es_first;		// first virtual page
	unsigned int es_npages;		// size of the segment in pages
	unsigned int es_nblocks;	// #initialized pages here in the file
	unsigned int es_prot;		// protection bits
};

#endif // EGOS_EXEC_H
