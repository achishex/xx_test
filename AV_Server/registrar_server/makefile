# Copyright (c) 2018 Xingluo.  All rights reserved.
#
#
ROOT_DIR=..
include ../makefile.comm

NAME = registrar_server
C++ = $(CPLUS)
CC = $(CCOMP)
#LINK = $(LINKER)
LINK = $(CXX)

CCFLAGS += $(COMPILER_FLAGS)  -Wall -Wno-format-y2k -std=c++11  -Wno-deprecated-declarations
CCFLAGS += -DHAVE_CONFIG_H  -Dregistrar_srv_conf
ROOTDIR = ..
# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
CCFLAGS += -I.
CCFLAGS += -I$(ROOTDIR)
CCFLAGS += -I$(ROOTDIR)/common
CCFLAGS += -I$(ROOTDIR)/thirdpart/db/build_unix
CCFLAGS += -I$(ROOTDIR)/thirdpart/cajun/include
CCFLAGS += -I$(ROOTDIR)/thirdpart/include
CCFLAGS += -I$(ROOTDIR)/common/util

# LIBS
LIBS = -L$(ROOTDIR)/thirdpart/db/build_unix -ldb_cxx
LIBS += -L$(ROOTDIR)/common/resip/dum -ldum
LIBS += -L$(ROOTDIR)/common/resip/stack -lresip
LIBS += -L$(ROOTDIR)/common/rutil -lrutil
LIBS += -L$(ROOTDIR)/common/rutil/dns/ares -lresipares
LIBS += $(CORE_LINK_LIBS)

# LINKOPTS
LINKOPTS = 


C++FLAGS = $(CCFLAGS)

CPP_COMM_FILES = $(ROOTDIR)/common/util/CBuffer.cpp
CPP_COMM_FILES += $(ROOTDIR)/common/util/unix_util.cpp

CPPFILES = 	repro.cxx					\
			RouteStore.cxx UserStore.cxx ConfigStore.cxx \
			AclStore.cxx StaticRegStore.cxx FilterStore.cxx SiloStore.cxx \
			Store.cxx AbstractDb.cxx BerkeleyDb.cxx \
			BasicWsConnectionValidator.cxx CommandServer.cxx \
			CommandServerThread.cxx ProxyConfig.cxx ReproVersion.cxx \
			HttpBase.cxx HttpConnection.cxx WebAdmin.cxx \
			WebAdminThread.cxx AccountingCollector.cxx Proxy.cxx \
			Registrar.cxx RegSyncClient.cxx RegSyncServer.cxx \
			RegSyncServerThread.cxx ReproRunner.cxx \
			ReproAuthenticatorFactory.cxx ReproRADIUSServerAuthManager.cxx \
			ReproServerAuthManager.cxx ReproTlsPeerAuthManager.cxx \
			RequestContext.cxx ResponseContext.cxx RRDecorator.cxx \
			Processor.cxx ProcessorChain.cxx SqlDb.cxx Target.cxx \
			UserAuthGrabber.cxx WorkerThread.cxx XmlRpcConnection.cxx \
			XmlRpcServerBase.cxx Dispatcher.cxx OutboundTarget.cxx \
			PersistentMessageQueue.cxx QValueTarget.cxx \
			stateAgents/PresenceServer.cxx \
			stateAgents/PresencePublicationHandler.cxx \
			stateAgents/PresenceSubscriptionHandler.cxx \
			monkeys/CertificateAuthenticator.cxx \
			monkeys/DigestAuthenticator.cxx \
			monkeys/CookieAuthenticator.cxx monkeys/StrictRouteFixup.cxx \
			monkeys/AmIResponsible.cxx monkeys/IsTrustedNode.cxx \
			monkeys/ConstantLocationMonkey.cxx monkeys/LocationServer.cxx \
			monkeys/OutboundTargetHandler.cxx \
			monkeys/RADIUSAuthenticator.cxx monkeys/RecursiveRedirect.cxx \
			monkeys/SimpleStaticRoute.cxx monkeys/StaticRoute.cxx \
			monkeys/QValueTargetHandler.cxx \
			monkeys/SimpleTargetHandler.cxx \
			monkeys/GeoProximityTargetSorter.cxx monkeys/RequestFilter.cxx \
			monkeys/MessageSilo.cxx \
			monkeys/RedirectServer.cxx \
            PolicySynClient.cxx t_socket.cxx t_proto.cxx \
	
# CPPFILES += stateAgents/CertServer.cxx \
# 	stateAgents/CertPublicationHandler.cxx \
# 	stateAgents/CertSubscriptionHandler.cxx \
# 	stateAgents/PrivateKeyPublicationHandler.cxx \
# 	stateAgents/PrivateKeySubscriptionHandler.cxx \

#CPPFILES += MySqlDb.cxx \
#CPPFILES += PostgreSqlDb.cxx \


SUBDIRS = ../common/rutil \
		../common/resip \

						
all: $(NAME)
	

$(NAME): $(CFILES:.c=.o) $(CPPFILES:.cxx=.o) $(CPP_COMM_FILES:.cpp=.o) $(LIBFILES)
	for dir in $(SUBDIRS);  do \
        $(MAKE) -C $$dir $*; \
	done
	$(CXX) -o $@ $(CFILES:.c=.o) $(CPPFILES:.cxx=.o) $(CPP_COMM_FILES:.cpp=.o) $(LINKOPTS) $(LIBS) 
	cp $(NAME) $(ROOTDIR)/bin
	cp repro.config $(ROOTDIR)/bin/reg_repro.config 
	cp users.txt $(ROOTDIR)/bin

install:
	@echo  "not implemented yet"


clean:
	rm -f $(NAME) $(CFILES:.c=.o) $(CPPFILES:.cxx=.o) $(CPP_COMM_FILES:.cpp=.o)
	rm -f $(ROOTDIR)/bin/$(NAME)
	for dir in $(SUBDIRS);  do \
		$(MAKE) -C $$dir clean; \
	done

Make_Subs:
	for dir in $(SUBDIRS);  do \
		$(MAKE) -C $$dir $*; \
	done

tar: 
	@echo  "not implemented yet"

include $(ROOTDIR)/Makefile.POSIX
