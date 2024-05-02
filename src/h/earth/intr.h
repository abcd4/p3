// Should be large enough to hold any context (ucontext_t->uc_mcontext)
#define SIGNAL_CONTEXT_SIZE		4096

/* Types of emulated interrupts.  Should all be self-explanatory.
 */
enum intr_type {
	INTR_PAGE_FAULT,
	INTR_CLOCK,
	INTR_SYSCALL,
	INTR_IO,
	INTR_SIZE					// this should be the last one
};

/* Interrupt context passed to interrupt handler.
 */
struct intr_context {
	enum intr_type type;	// type of interrupt
    void *context;          // process context (registers)
    unsigned int ctxsize;   // size of context
	void (*ip)(void);		// instruction pointer (program counter)
	void *sp;				// stack pointer
};

/* Type of interrupt handler.
 */
typedef void (*intr_handler_t)(struct intr_context *, void *);

/* Interface to the Earth interrupt module.
 */
struct intr_intf {
	void (*initialize)(intr_handler_t handler);
	void (*suspend)(unsigned int maxtime);
	void (*register_dev)(int fd, void (*read_avail)(void *), void *arg);
#ifdef OBSOLETE
	void (*save)(void *p, void *sp);
	void (*restore)(void *p, void *sp);
#endif
	void (*sched_event)(void (*handler)(void *arg), void *arg);
};

void intr_setup(struct intr_intf *ii);
