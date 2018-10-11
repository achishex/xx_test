# Copyright (c) 2018 Xingluo.  All rights reserved.
# 
AR= ar
RANLIB= ranlib

NAME = resipares
C++ = $(CPLUS)
CC = $(CCOMP)
LINK = $(LINKER)
CCFLAGS += $(COMPILER_FLAGS)  -Wall
CCFLAGS += -DHAVE_CONFIG_H
ROOTDIR = ../../../..
# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
CCFLAGS += -I.
CCFLAGS += -I$(ROOTDIR)
CCFLAGS += -I$(ROOTDIR)/common
CCFLAGS += -fPIC

# LIBS
LIBS = $(CORE_LINK_LIBS) $(SHARED)

# LINKOPTS
LINKOPTS =  

C++FLAGS = $(CCFLAGS)

CFILES  = \
	ares__close_sockets.c ares__get_hostent.c ares__read_line.c \
	ares_destroy.c ares_expand_name.c ares_fds.c ares_free_errmem.c \
	ares_free_hostent.c ares_free_string.c ares_gethostbyaddr.c \
	ares_gethostbyname.c ares_init.c ares_mkquery.c ares_parse_a_reply.c \
	ares_parse_ptr_reply.c ares_process.c ares_query.c ares_search.c \
	ares_send.c ares_strerror.c ares_timeout.c ares_local.c \

CPPFILES =  


# some libs need to be compiled!!
SUBDIRS =  


all: lib$(NAME).a #lib$(NAME).so


lib$(NAME).so: make_subs $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)  $(LIBFILES)
	$(LINK) -o $@ $(CFILES:.c=.o) $(CPPFILES:.cxx=.o) $(COMPILER_FLAGS) $(LINKOPTS) $(LIBS) 
	cp lib$(NAME).so $(ROOTDIR)/bin		

lib$(NAME).a: make_subs $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)
	$(AR) -r $@ $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)
	$(RANLIB) $@		
	#cp lib$(NAME).a $(ROOTDIR)/bin		

make_subs:
	for dir in $(SUBDIRS);  do \
		$(MAKE) -C $$dir  $*; \
	done


install: $(NAME)
	@echo  "not implemented yet"

clean: clean_subs
	rm -f lib$(NAME).so lib$(NAME).a $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)
	rm -f $(ROOTDIR)/bin/lib$(NAME).so 
	rm -f $(ROOTDIR)/bin/lib$(NAME).a 

clean_subs:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

include $(ROOTDIR)/Makefile.POSIX
