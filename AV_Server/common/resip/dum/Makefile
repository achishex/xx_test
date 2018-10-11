# Copyright (c) 2018 Xingluo.  All rights reserved.
# 
AR= ar
RANLIB= ranlib
#
NAME = dum
C++ = $(CPLUS)
CC = $(CCOMP)
LINK = $(LINKER)
CCFLAGS += $(COMPILER_FLAGS)  -Wall
CCFLAGS += -DHAVE_CONFIG_H
ROOTDIR = ../../..
# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS
CCFLAGS += -I.
CCFLAGS += -I$(ROOTDIR)
CCFLAGS += -I$(ROOTDIR)/common
CCFLAGS += -fPIC

# LIBS
LIBS = -L../stack -lresip
LIBS += -L../../rutil -lrutil
LIBS += -L../../rutil/dns/ares -lresipares
LIBS += $(CORE_LINK_LIBS) $(SHARED)

# LINKOPTS
LINKOPTS =  

C++FLAGS = $(CCFLAGS)

CFILES  = 

CPPFILES =   \
	AppDialog.cxx \
	AppDialogSet.cxx \
	AppDialogSetFactory.cxx \
	BaseCreator.cxx \
	BaseUsage.cxx \
	UserAuthInfo.cxx \
	BaseSubscription.cxx \
	ChallengeInfo.cxx \
	ClientAuthManager.cxx \
	ClientAuthExtension.cxx \
	ClientInviteSession.cxx \
	ClientOutOfDialogReq.cxx \
	ClientPagerMessage.cxx \
	ClientPublication.cxx \
	ClientRegistration.cxx \
	ClientSubscription.cxx \
	ContactInstanceRecord.cxx \
	DefaultServerReferHandler.cxx \
	DestroyUsage.cxx \
	Dialog.cxx \
	DialogEventInfo.cxx \
	DialogEventStateManager.cxx \
	DialogId.cxx \
	DialogSet.cxx \
	DialogSetId.cxx \
	DialogUsage.cxx \
	DialogUsageManager.cxx \
	DumProcessHandler.cxx \
	DumThread.cxx \
	DumTimeout.cxx \
	EncryptionRequest.cxx \
	HandleException.cxx \
	HandleManager.cxx \
	Handle.cxx \
	Handled.cxx \
	InMemoryRegistrationDatabase.cxx \
	InMemorySyncPubDb.cxx \
	InMemorySyncRegDb.cxx \
	InviteSession.cxx \
	InviteSessionCreator.cxx \
	InviteSessionHandler.cxx \
	MergedRequestKey.cxx \
	NonDialogUsage.cxx \
	OutOfDialogReqCreator.cxx \
	PagerMessageCreator.cxx \
	MasterProfile.cxx \
	UserProfile.cxx \
	Profile.cxx \
	PublicationCreator.cxx \
	RADIUSServerAuthManager.cxx \
	RedirectManager.cxx \
	RegistrationCreator.cxx \
	RegistrationHandler.cxx \
	ServerAuthManager.cxx \
	ServerInviteSession.cxx \
	ServerOutOfDialogReq.cxx \
	ServerPagerMessage.cxx \
	ServerPublication.cxx \
	ServerRegistration.cxx \
	ServerSubscription.cxx \
	SubscriptionHandler.cxx \
	SubscriptionCreator.cxx \
	SubscriptionState.cxx \
    TlsPeerAuthManager.cxx \
    WsCookieAuthManager.cxx \
	KeepAliveManager.cxx \
	KeepAliveTimeout.cxx \
	NetworkAssociation.cxx \
	DumDecrypted.cxx \
	CertMessage.cxx \
	DumFeatureChain.cxx \
	DumFeatureMessage.cxx \
	IdentityHandler.cxx \
	TargetCommand.cxx \
	DumFeature.cxx \
	OutgoingEvent.cxx \
	HttpProvider.cxx \
	HttpGetMessage.cxx \
	DumHelper.cxx \
	MergedRequestRemovalCommand.cxx \

# CPPFILES += ssl/EncryptionManager.cxx \


# some libs need to be compiled!!
SUBDIRS =  \


all: lib$(NAME).a # lib$(NAME).so

lib$(NAME).so: make_subs $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)  $(LIBFILES)
	$(LINK) -o $@ $(CFILES:.c=.o) $(CPPFILES:.cxx=.o) $(COMPILER_FLAGS) $(LINKOPTS) $(LIBS) 
	cp lib$(NAME).so $(ROOTDIR)/bin	

lib$(NAME).a: make_subs $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)
	$(AR) cr $@ $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)
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
