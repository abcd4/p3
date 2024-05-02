The following only works well under Linux

1. set WORKDIR=`pwd`
2. In src/tcc, run ./configure --prefix=$WORKDIR/build/tcc in order to create folder build/tcc that contains tcc binary files
3. In src/tcc, run "make", "make test", and "make install"
		(ignore the various errors...)
4. In EGOS directory, run "make -f src/make/Makefile.apps tcc_install"
