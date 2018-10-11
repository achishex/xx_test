# Copyright (c) 2018 Xingluo.  All rights reserved.
# 

AR= ar
RANLIB= ranlib

NAME = rutil
C++ = $(CPLUS)
CC = $(CCOMP)
LINK = $(LINKER)
CCFLAGS += $(COMPILER_FLAGS)  -Wall
CCFLAGS += -DHAVE_CONFIG_H
ROOTDIR = ../..
# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
CCFLAGS += -I.
CCFLAGS += -I$(ROOTDIR)
CCFLAGS += -I$(ROOTDIR)/common
CCFLAGS += -I./dns/ares
CCFLAGS += -fPIC

# LIBS
LIBS = -L./dns/ares -lresipares
LIBS += $(CORE_LINK_LIBS) $(SHARED)

# LINKOPTS
LINKOPTS =  

C++FLAGS = $(CCFLAGS)

CFILES  = 

CPPFILES =   \
	AbstractFifo.cxx \
	AndroidLogger.cxx \
	BaseException.cxx \
	Coders.cxx \
	Condition.cxx \
	ConfigParse.cxx \
	CountStream.cxx \
	ServerProcess.cxx \
	Data.cxx \
	DataStream.cxx \
	DnsUtil.cxx \
	FileSystem.cxx \
	GeneralCongestionManager.cxx \
	HeapInstanceCounter.cxx \
	KeyValueStore.cxx \
	Lock.cxx \
	Log.cxx \
	MD5Stream.cxx \
	Mutex.cxx \
	NetNs.cxx \
	ParseBuffer.cxx \
	ParseException.cxx \
	Poll.cxx \
	PoolBase.cxx \
	FdPoll.cxx \
	RADIUSDigestAuthenticator.cxx \
	RWMutex.cxx \
	Random.cxx \
	RecursiveMutex.cxx \
	resipfaststreams.cxx \
	SelectInterruptor.cxx \
	Sha1.cxx \
	Socket.cxx \
	Subsystem.cxx \
	SysLogBuf.cxx \
	SysLogStream.cxx \
	ThreadIf.cxx \
	Time.cxx \
	Timer.cxx \
	TransportType.cxx \
	vmd5.cxx \
	XMLCursor.cxx \
	\
	dns/AresDns.cxx \
	dns/DnsCnameRecord.cxx \
	dns/DnsAAAARecord.cxx \
	dns/DnsHostRecord.cxx \
	dns/DnsNaptrRecord.cxx \
	dns/DnsResourceRecord.cxx \
	dns/DnsSrvRecord.cxx \
	dns/DnsStub.cxx \
	dns/DnsThread.cxx \
	dns/ExternalDnsFactory.cxx \
	dns/RRCache.cxx \
	dns/RRList.cxx \
	dns/RRVip.cxx \
	dns/QueryTypes.cxx \
	dns/RROverlay.cxx \
	\
	stun/Stun.cxx \
	stun/Udp.cxx \

# CPPFILES += ssl/OpenSSLInit.cxx \
# 	ssl/SHA1Stream.cxx \


# some libs need to be compiled!!
SUBDIRS = ./dns/ares \


all: lib$(NAME).a #lib$(NAME).so 


lib$(NAME).so: make_subs $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)  $(LIBFILES)
	$(LINK) -o $@ $(CFILES:.c=.o) $(CPPFILES:.cxx=.o) $(COMPILER_FLAGS) $(LINKOPTS) $(LIBS) 
	cp lib$(NAME).so $(ROOTDIR)/bin		

lib$(NAME).a: make_subs $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)
	$(AR) cr $@ $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)
	$(RANLIB) $@			
	#cp $@ $(ROOTDIR)/bin

make_subs:
	for dir in $(SUBDIRS);  do \
		$(MAKE) -C $$dir  $*; \
	done


install: $(NAME)
	@echo  "not implemented yet"

clean: clean_subs
	rm -f lib$(NAME).so  lib$(NAME).a $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)
	rm -f $(ROOTDIR)/bin/lib$(NAME).so 
	rm -f $(ROOTDIR)/bin/lib$(NAME).a

clean_subs:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

include $(ROOTDIR)/Makefile.POSIX
