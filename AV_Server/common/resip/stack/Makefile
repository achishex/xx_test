# Copyright (c) 2018 Xingluo.  All rights reserved.
# 

AR= ar
RANLIB= ranlib

NAME = resip
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
LIBS = -L../../rutil -lrutil
LIBS += -L../../rutil/dns/ares -lresipares
LIBS += $(CORE_LINK_LIBS) $(SHARED)

# LINKOPTS
LINKOPTS =  

C++FLAGS = $(CCFLAGS)

CFILES  = 

CPPFILES =   \
	Auth.cxx \
	CSeqCategory.cxx \
	CallId.cxx \
	Cookie.cxx \
	DateCategory.cxx \
	gen/DayOfWeekHash.cxx \
	DtmfPayloadContents.cxx \
	ExpiresCategory.cxx \
	GenericUri.cxx \
	IntegerCategory.cxx \
	InteropHelper.cxx \
	UInt32Category.cxx \
	Mime.cxx \
	NameAddr.cxx \
	ParserCategories.cxx \
	RequestLine.cxx \
	StatusLine.cxx \
	StringCategory.cxx \
	Token.cxx \
	TokenOrQuotedStringCategory.cxx \
	Via.cxx \
	WarningCategory.cxx \
	Aor.cxx \
	ApiCheck.cxx \
	ApplicationSip.cxx \
	BasicNonceHelper.cxx \
	BranchParameter.cxx \
	Connection.cxx \
	ConnectionBase.cxx \
	ConnectionManager.cxx \
	Contents.cxx \
	ContentsFactoryBase.cxx \
	CpimContents.cxx \
	DataParameter.cxx \
	DeprecatedDialog.cxx \
	DnsInterface.cxx \
	DnsResult.cxx \
	DtlsMessage.cxx \
	Embedded.cxx \
	ExtensionParameter.cxx \
	ExtensionHeader.cxx \
	ExistsOrDataParameter.cxx \
	ExistsParameter.cxx \
	ExternalBodyContents.cxx \
	QValue.cxx \
	QValueParameter.cxx \
	GenericContents.cxx \
	GenericPidfContents.cxx \
	HeaderFieldValue.cxx \
	HeaderFieldValueList.cxx \
	gen/HeaderHash.cxx \
	HeaderTypes.cxx \
	Headers.cxx \
	Helper.cxx \
	IntegerParameter.cxx \
	UInt32Parameter.cxx \
	InternalTransport.cxx \
	LazyParser.cxx \
	Message.cxx \
	MessageWaitingContents.cxx \
	gen/MethodHash.cxx \
	MethodTypes.cxx \
	gen/MonthHash.cxx \
	MsgHeaderScanner.cxx \
	MultipartAlternativeContents.cxx \
	MultipartMixedContents.cxx \
	MultipartRelatedContents.cxx \
	MultipartSignedContents.cxx \
	NonceHelper.cxx \
	OctetContents.cxx \
	Parameter.cxx \
	gen/ParameterHash.cxx \
	ParameterTypes.cxx \
	ParserCategory.cxx \
	ParserContainerBase.cxx \
	Pidf.cxx \
	Pkcs7Contents.cxx \
	Pkcs8Contents.cxx \
	PlainContents.cxx \
	PrivacyCategory.cxx \
	QuotedDataParameter.cxx \
	RAckCategory.cxx \
	Rlmi.cxx \
	RportParameter.cxx \
	SERNonceHelper.cxx \
	SdpContents.cxx \
	SecurityAttributes.cxx \
	Compression.cxx \
	SipFrag.cxx \
	SipMessage.cxx \
	SipStack.cxx \
	StackThread.cxx \
	InterruptableStackThread.cxx \
	EventStackThread.cxx \
	StatisticsHandler.cxx \
	StatisticsManager.cxx \
	StatisticsMessage.cxx \
	Symbols.cxx \
	TcpBaseTransport.cxx \
	TcpConnection.cxx \
	TcpConnectState.cxx \
	TcpTransport.cxx \
	TimeAccumulate.cxx \
	TimerMessage.cxx \
	TimerQueue.cxx \
	Tuple.cxx \
	TupleMarkManager.cxx \
	TransactionController.cxx \
	MessageFilterRule.cxx \
	TransactionUser.cxx \
	TransactionUserMessage.cxx \
	TransactionMap.cxx \
	TransactionState.cxx \
	Transport.cxx \
	TransportThread.cxx \
	TransportFailure.cxx \
	TransportSelector.cxx \
	TuIM.cxx \
	TuSelector.cxx \
	UdpTransport.cxx \
	UnknownParameter.cxx \
	Uri.cxx \
	X509Contents.cxx \
	KeepAliveMessage.cxx \
	StatelessHandler.cxx \
	InvalidContents.cxx \
	WsBaseTransport.cxx \
	WsFrameExtractor.cxx \
	WsTransport.cxx \
	WsConnection.cxx \
	WsConnectionBase.cxx \
	WsCookieContext.cxx \
	WsDecorator.cxx \

# CPPFILES += ssl/DtlsTransport.cxx \
# 	ssl/Security.cxx \
# 	ssl/TlsBaseTransport.cxx \
# 	ssl/TlsConnection.cxx \
# 	ssl/TlsTransport.cxx \
# 	ssl/WssTransport.cxx \
# 	ssl/WssConnection.cxx \

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
	rm -f lib$(NAME).so  lib$(NAME).a $(CFILES:.c=.o) $(CPPFILES:.cxx=.o)
	rm -f $(ROOTDIR)/bin/lib$(NAME).so 
	rm -f $(ROOTDIR)/bin/lib$(NAME).a

clean_subs:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

include $(ROOTDIR)/Makefile.POSIX
