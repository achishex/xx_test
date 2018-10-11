ROOT_DIR= ..
include ../makefile.comm

INC_SVR_COMM := -I.  \
    -I/usr/include \
    -I/usr/local/include \
    -I$(ThirdPathInclude) \
    -I$(UtilDir) \
    -I$(ProtoInnerDir) \
    -I$(RedisClientDir) \
    -I$(CommonDir) \
    -I$(LibHiredisInclude) \
    -I$(LibrapidxmlInclude) \
    -I$(LibeventInclude) \
    -I$(Log4CppInclude) \
    -I$(RapidJsonDir) 

LIB_SVR_COMM := \
    -L$(ThirdPathLib) \
    -L$(LibeventLib) -levent -levent_pthreads \
    -L$(LibHiredisLib) -lhiredis \
    -L$(Log4CppLib) -llog4cpp \
    -L$(LibTcmallocLib) -ltcmalloc -lunwind \
    -L$(SYSTEM_LIB_PATH) \
    -lpthread -lz -lc -ldl -lrt
    

INC_ALL := $(INC_SVR_COMM) 
LIB_ALL := $(LIB_SVR_COMM)

BINARY = policy_server 

all:$(BINARY)

CXXFLAGS += #-D_debug_register #-Dno_daemon

.SUFFIXES: .o .cpp
%.o:%.cpp
	$(CXX) $(INC_ALL) $(CXXFLAGS) -c $< -o $@  

COMM_PATH=$(UtilDir) $(RedisClientDir)

THIRD_PARTY_PATH=$(Log4CppInclude) \
                 $(Log4CppLib)    \
                 $(RapidJsonDir) 


CPP_COMMON_SRC=$(foreach dir, $(COMM_PATH), $(wildcard $(dir)/*.cpp))
OBJ_COMMON=$(patsubst %.cpp,%.o,$(CPP_COMMON_SRC))

CPP_THIRD_PART_PATH_SRC=$(foreach dir, $(THIRD_PARTY_PATH), $(wildcard $(dir)/*.cpp))
OBJ_THIRD_PART_PATH=$(patsubst %.cpp,%.o,$(CPP_THIRD_PART_PATH_SRC))

DIRS=.
CPP_SRCS= $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
OBJ_CUR=$(patsubst %.cpp,%.o,$(CPP_SRCS)) $(OBJ_UTIL) $(OBJ_COMMON_CFG_PARSE) $(OBJ_COMM_LOG)

OBJ += $(OBJ_COMMON)
OBJ += $(OBJ_THIRD_PART_PATH)
OBJ += $(OBJ_CUR) 

policy_server:$(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJ) $(LIB_ALL) -o $@
	@echo "cp policy_server to ../bin"
	@cp  policy_server ../bin >>/dev/null 2>&1
clean:
	rm -f *.o *~ $(BINARY)
	rm -f $(OBJ) policy_server.lock
	rm -f ../bin/policy_server
	rm -f ../bin/policy_server.lock

strip:
	strip $(BINARY)
