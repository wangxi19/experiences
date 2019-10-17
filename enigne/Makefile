INCLUDE += -I$(PWD)/../Public/include/ -I./ -I/root/Documents/Public/include/ -I./capture/ -I./analysis/
CC = gcc
CXX = g++
CPPFLAGS = -std=c++11
ifneq ($(dbg), 1)
    CPPFLAGS := $(CPPFLAGS) -O3
else
    CPPFLAGS += -g3 -O0
endif

LIBS += -L./capture/ -l:libpcapcapture.a -L./analysis/ -l:libanalysispkt.a -L./logtool/ -l:liblogtool.a  -lpthread -L/usr/lib64/ -ldbus-1 -L/usr/local/lib/ -l:libpcap.a -L$(PWD)/libs/ -l:libPacketDecoder.so
LDFLAGS +=

CPPFLAGS += $(INCLUDE)
CPPFLAGS += $(LIBS)
CPPFLAGS += $(LDFLAGS)

SRCS = $(wildcard *.cpp)
#OBJS = $(patsubst %.cpp, %.o, $(SRCS))
OBJS = $(SRCS:%.cpp=%.o)

TARGET = main

all: $(TARGET)

$(TARGET): $(filter %.o, $(OBJS))
#	$(CXX) -shared $(CPPFLAGS) -o $@.so $^
#	$(AR) -r $(TARGET).a $^
	cd ./analysis/ && make dbg=1 && cd ../
	cd ./capture/ && make dbg=1 && cd ../
	cd ./logtool/ && make dbg=1 && cd ../
	$(CXX) -o $(TARGET) $^ $(CPPFLAGS)

$(filter %.o, $(OBJS)): %.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(LDFLAGS) $< -o $@

.PHONY: clean

clean: 
	$(RM) $(OBJS)
	$(RM) $(TARGET)
