This is the CS4411 operating system development environment called
EGOS (for Earth and Grass Operating System).  It runs in user space,
but paging is more-or-less real.  In the 'earth' directory are the
sources for a simulated hardware environment that includes a
software-loaded TLB and interrupt handling.  The 'grass' directory
implements the operating system on top of this simulated hardware.

Running this
============

Simply run:

    make run

Hopefully a login prompt will come up, and you can log in to the guest
account "guest" using password "guest".
Then you can run commands like 'ls', 'echo', and 'cat', or even run a
nested 'shell'.  Do `ls /bin` to see a list of all commands.  There is
even a C compiler!!

`make run` puts the terminal temporarily in 'raw' mode, meaning that input
characters are passed as is, and then runs "build/earth/earthbox", the executable.
You can run "earthbox" directly yourself, but remember that the terminal
will then be in "cooked" mode and input/output will feel unnatural.

You can also do

	make run2

The difference between "make run" and "make run2" is that the latter always
creates a new file system from scratch, which takes a while, whereas the former
only creates a new filesystem the first time it is called.  The filesystem in 
EGOS is initialized by copying everything in the directory in which you invoke 
`make run`, but it does not automatically stay synchronized with this directory.
Thus, if you changed or added applications or libraries and want to see them 
within EGOS, you have to do either "make run2" or, to be on the safe side:

	make clean
	make run

Debugging
=========

If you type ctrl-L, the EGOS kernel will dump a listing of the running
processes.  If you type ctrl-Q, the EGOS kernel will immediately exit.
Note, however, that the file system cache may not be synced.  You may want
to run "sync" first, or call "shutdown" instead.

You can run the Earth virtual machine in gdb, to do this, do:

    $ gdb build/earth/earthbox
    (gdb) handle SIGSEGV nostop noprint
    (gdb) handle SIGILL nostop noprint
    (gdb) r

The reason for the two handles is that we don't want gdb to intercept
the SIGSEGV signals that are used for page faulting, or the SIGILL
signals that are used for system calls.  (We use instruction '.long 6',
which is not an x86 instruction, as a system call instruction.) On
Mac OS X you probably also need to do the same for SIGBUS, as page
faults manifest themselves there that way.

You can set breakpoints, single step through code, print the values
of global and stack variables, see the stack trace, and so on.
Unfortunately, it is not possible to catch illegal accesses by the
kernel to memory, because those generate SIGSEGV, which we told the
debugger to ignore.  Instead, the 'earth' subsystem will print a report.
Also unfortunately, it is not really possible to run valgrind with the
operating system (to find uninitialized variables and/or memory leaks)
because valgrind gets confused by all the stack operations.

Directory Organization
======================
    bin        EGOS binaries, all ending in .exe
    build      Where everything is built from sources
    docs       Documents
    etc        Mostly used to contain the password file
    lib        EGOS libraries
    releases   EGOS releases
    src        All the sources are kept here
    storage    Images of simulated disks
    tcc        Tiny C Compiler object files
    usr        Contains EGOS user directories

Mounting new file systems
=========================

You can run your own block server and file server in user space.
To do so, first log in and then run:

	blocksvr -r 1024&

This starts a block server in the background with a ramdisk consisting
of 1024 blocks and some other layers on top of that (see blocksvr source).

Then start a block file server in the background:

	bfs PID1&

Here PID1 is the process identifier of the block server.

Finally, you will probably want to mount the block file server in your
current file system.  To do that, do:

	mount NAME PID2

This creates a directory NAME.dir, and if the file server works, you should
be able to use it.

Other handy commands
====================
    ed              A simple file editor inspired by Unix ed
    elf_cvt         Converts ELF binaries to EGOS binaries
    loop            Runs a long or even infinite loop for testing
    pull            "pull X Y" reads file X from the underlying operating system
                    and stores it as Y in EGOS.  If you don't specify Y, it prints
                    the file to standard output.
    shell           The shell.  Doesn't support much...
    shutdown        Cleanly shutdown EGOS.
    tcc             Tiny C compiler.  cc is a wrapper around tcc.

Understanding Output of Ctrl-L
==================================

You may see something like the following:

    200 free frames; 962 free blocks; npgout = 23; npgin = 85
    15 processes (current = 2, nrunnable = 0, load = 0.82):
    PID   DESCRIPTION  UID STATUS      RES SWP LEVEL QUANT OWNER ALARM   EXEC
       1: K main         0 AWAIT EVENT   0   0     0    10     1
       2: K tty          0 AWAIT REQST   0   0     0    10     1
       3: K spawn        0 AWAIT REQST   0   0     0    10     1
       4: K gate         0 AWAIT REQST   0   0     0    10     1
       5: K ramfile      0 AWAIT REQST   0   0     0    10     1
       6: K page.dev     0 AWAIT REQST   0   0     0    10     1
       7: K fs.dev       0 AWAIT REQST   0   0     0    10     1
       8: U blocksvr     0 AWAIT REQST  18   2     0    10     1         5:1
       9: U bfs          0 AWAIT REQST  15   2     0    10     1         5:2
      10: U dirsvr       0 AWAIT REQST  11   1     0    10     1         5:3
      11: U init         0 AWAIT EVENT   0  16     0    10     1         5:4
      12: U syncsvr      0 AWAIT REQST   0  10     0    10    11 22512   9:18
      13: U pwdsvr       0 AWAIT REQST   0  12     0     9    11         9:16
      14: U login        0 AWAIT EVENT   0  19     0    10    11         9:15
      15: U shell       10 AWAIT     2  12   0     0    10    14         9:38

The first line

	200 free frames; 962 free blocks; npgout = 23; npgin = 85

says that there are 200 unallocated memory frames for paging, and
962 unused blocks that can be used for paging.  23 frames have been
paged out, 85 have been paged back in.

The second line

    15 processes (current = 2, nrunnable = 0, load = 0.82):

says that there are 15 processes.  The "current" process is process
2.  There are no runnable processes.  The load average (average
length of the run queue) is 0.82.

Then there is a line for each process.  For example

    PID   DESCRIPTION  UID STATUS      RES SWP LEVEL QUANT OWNER ALARM   EXEC
      12: U syncsvr      0 AWAIT REQST   0  10     0    10    11 22512   9:18

says that process 12 is a user process that is running "syncsvr".
Its user ID is 0.  It is awaiting a request.  0 pages are resident
in frames, 10 pages have been paged out.  It is in level 0 of the
multi-level feedback scheduling queue, and uses quanta of 10 clock
ticks.  Its parent process is 11 (init).  It's waiting for 22512
milliseconds to expire.  Its executable file lives on file server
9 (which happens to be bfs), and its inode number is 18.

Other types of STATUS include:

    AWAIT 2:            wait for a response from process 2 (tty)
    AWAIT EVENT:        waiting for an event

The log
=======

The file log.txt maintains a log of events that happen in the kernel.
It can be useful for understanding what's going on and also for debugging.

Using the editor
================

The ed editor is a very small editor much inspired by the Unix ed editor.
The basic commands are:

	p:			print the current line
	-:			print the previous line
	w:			save the file
	q:			quit
	Q:			quit without saving the file
	i:			insert typed input before the current line
	a:			append typed input after the current line
	c:			replace the current line
	d:			delete the current line

Some of these commands can be preceded by one or two "positions".  For example,
the p, i, a, c, and d commands can be preceded by a single position:  "3p" prints
line 3.  Moreover, the 'p', 'c', and 'd' commands can be preceded two positions
defining a range:  "3,5p" prints lines 3 through 5.  Besides numeric positions,
there are the following other options:

    $:          last line (e.g., "$p" prints the last line)
    .:          current line
    /string/    the next line containing "string" (will wrap around)

These can be followed by "+n" or "-n".  E.g., ".+3,$-3p$ prints the lines
starting at 3 lines beyond the current through 3 before the last line.

There are some shortcuts for ranges as well:

    g:          all lines.  E.g. "gp" prints all lines.
    z:          the next 10 lines
