all:
	chmod +x src/make/sname
	$(MAKE) -f src/make/Makefile.apps all
	$(MAKE) -f src/make/Makefile.grass all
	$(MAKE) -f src/make/Makefile.earth all

run: all storage/fs.dev
	@echo '============================'
	@echo '>>> hit <ctrl>q to exit  <<<'
	@echo '>>> hit <ctrl>l for dump <<<'
	@echo '============================'
	stty raw -echo; build/earth/earthbox; stty cooked echo

run2: all
	@echo '============================'
	@echo '>>> hit <ctrl>q to exit  <<<'
	@echo '>>> hit <ctrl>l for dump <<<'
	@echo '============================'
	rm -f storage/fs.dev build/tools/mkfs
	$(MAKE) -f src/make/Makefile.apps build/tools/mkfs
	build/tools/mkfs .
	stty raw -echo; build/earth/earthbox; stty cooked echo

storage/fs.dev: build/tools/mkfs
	build/tools/mkfs .

build/tools/mkfs:
	$(MAKE) -f src/make/Makefile.apps build/tools/mkfs

build/tools/cpr: src/tools/cpr.c
	$(CC) -o build/tools/cpr src/tools/cpr.c

cache_test:
	$(MAKE) -f src/make/Makefile.cache_test

fat_test:
	$(MAKE) -f src/make/Makefile.fat_test

clean:
	rm -f a.out build/earth/earthbox build/grass/k.int build/grass/k.out archive.c storage/*.dev storage/log.txt bin/*.exe lib/*.o lib/*.a build/tools/mkfs build/tools/cpr
	rm -fR build/tools/*_cvt*
	rm -f build/*/*.o build/*/*.d build/*/*.exe build/*/*.int build/*/*.a
	find . -name '*.log' -exec rm -f '{}' ';'
	find . -name '*.sav*' -exec rm -f '{}' ';'
	find . -name '*.aux' -exec rm -f '{}' ';'
	find . -name 'paper.pdf' -exec rm -f '{}' ';'
	$(MAKE) -f src/make/Makefile.cache_test clean
	$(MAKE) -f src/make/Makefile.fat_test clean
