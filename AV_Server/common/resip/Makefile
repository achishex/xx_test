SUBDIRS = stack dum 
ROOTDIR = ../..
all:
	for dir in $(SUBDIRS);  do \
		$(MAKE) -C $$dir -f Makefile ; \
	done
clean:
	for dir in $(SUBDIRS);  do \
		$(MAKE) -C $$dir -f Makefile clean; \
	done
install:
	for dir in $(SUBDIRS);  do \
		$(MAKE) -C $$dir -f Makefile install; \
	done


include $(ROOTDIR)/Makefile.POSIX
