INCLUDE += -I$(PWD)/../../Public/include/
CC = gcc
CXX = g++
CPPFLAGS = -std=c++11
ifneq ($(dbg), 1)
    CPPFLAGS := $(CPPFLAGS) -O3
else
    CPPFLAGS += -g3 -O0
endif

LIBS +=
LDFLAGS +=

CPPFLAGS += $(INCLUDE)
CPPFLAGS += $(LIBS)
CPPFLAGS += $(LDFLAGS)

SRCS = $(wildcard *.cpp)
#OBJS = $(patsubst %.cpp, %.o, $(SRCS))
OBJS = $(SRCS:%.cpp=%.o)

TARGET = libanalysispkt

all: $(TARGET)

$(TARGET): $(filter %.o, $(OBJS))
#	$(CXX) -shared $(CPPFLAGS) -o $@.so $^
	$(AR) -r $(TARGET).a $^

$(filter %.o, $(OBJS)): %.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(LDFLAGS) $< -o $@

.PHONY: clean

clean: 
	$(RM) $(OBJS)
	$(RM) $(TARGET).a
