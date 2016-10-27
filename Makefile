SubComCpp=
SrcCpp:=main.cpp server.cpp httpclient.cpp threaddata.cpp dealbase.cpp\
deal_auth.cpp deal_cmdreq.cpp deal_cmdresp.cpp deal_userlog.cpp idmanage.cpp\
comm/strparse.cpp comm/log.cpp comm/config.cpp comm/bmsh_config.cpp comm/lock.cpp\
comm/taskpool.cpp comm/file.cpp comm/sock.cpp comm/base64.cpp comm/md5.cpp\
tinyxml/bmshxml.cpp\
tinyxml/tinystr.cpp tinyxml/tinyxmlerror.cpp tinyxml/tinyxml.cpp tinyxml/tinyxmlparser.cpp\
redis/redis.cpp redis/redispool.cpp redis/redispooladmin.cpp\
mongo/mongo.cpp mongo/mongopool.cpp mongo/mongopooladmin.cpp

TARGET:=cnodeconnect


MKLLOGFILE:=make.log
CONF_HEAD_INC+=-DLOG2STDOUT#日志输出至标准输出
#CONF_HEAD_INC+=-DDEBUG -DHTTP_GET
LIBEVENT=/home/hejl/work/libevent2.0.22
LIBBSON=/usr/local/include/libbson-1.0
LIBMONGODB=/usr/local/include/libmongoc-1.0

INCLUDEDIR+= -I. -I$(LIBEVENT)/include/ -I$(LIBMONGODB) -I$(LIBBSON)
CXX=g++
CXXFLAGS=-Wall -g 

CPPFLAGS+=$(CONF_HEAD_INC) $(INCLUDEDIR)
LDFLAGS=-Wall
LDLIBS= #-l100mshextphp 
LOADLIBES=-L$(FCGI_DIR)/lib -L$(LIBEVENT)/lib
ALIBS=

# 以下是使用静态库时选项
LOADLIBES+=-L/usr/local/lib 
LDLIBS+=-lpthread -levent -lhiredis -lmongoc-1.0 # -ljsoncpp

SRC=$(SrcCpp)
OBJ=$(SRC:.cpp=.o)

# linking operation from *.o and lib.
$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) $^ $(ALIBS) -o $@ $(LOADLIBES) $(LDLIBS) -Wl,-rpath,./:/usr/local/lib/bmsh
	@echo 
	@echo "make $(TARGET) ok."  $(CONF_DESCRIPTION)
	@echo `date "+%Y%m%d %H:%M:%S"` :  `md5sum $(TARGET)` >> $(MKLLOGFILE)
	@echo $(CONF_DESCRIPTION) >> $(MKLLOGFILE)
	@echo >> $(MKLLOGFILE)
	

#test code # 增加测试程度像如下test1样式添加
test1:  $(patsubst main.o,test1.o,$(OBJ))
	$(CXX) $(LDFLAGS) $^ $(ALIBS) -o $@ $(LOADLIBES) $(LDLIBS)
test2:  $(patsubst main.o,test2.o,$(OBJ))
	$(CXX) $(LDFLAGS) $^ $(ALIBS) -o $@ $(LOADLIBES) $(LDLIBS)

# ---------------------------------------- #
sinclude .depend
.PHONY: clean depend
clean:
	@rm -rf $(TARGET) $(OBJ) $(COMMON_OBJ) 

depend: $(SRC)
	$(CXX) -MM $< $(INCLUDEDIR) $^ > .depend

