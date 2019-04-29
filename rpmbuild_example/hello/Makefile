CPPFLAGS = -std=c++11 -pthread

ifneq ($(dbg), 1)
	CPPFLAGS += -O3
else
	CPPFLAGS := $(CPPFLAGS) -g -O0
endif

CXX =g++
SRCS =$(wildcard ./src/*.cpp)
OBJS =$(SRCS:./src/%.cpp=%.o)
LIBS +=

CPPFLAGS += $(LIBS)
CPPFLAGS += -I./src/

PREFIX ?= /usr/local

TARGET ?= myhello

all: $(TARGET)


$(TARGET): $(filter %.o, $(OBJS))
	$(CXX) $(CPPFLAGS) $^ -o $@	


$(filter %.o, $(OBJS)): %.o: ./src/%.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $@

.PHONY: clean

clean:
	$(RM) $(TARGET) 
	$(RM) $(filter $(wildcard *.o), $(OBJS))
	$(RM) $(PREFIX)/bin/$(TARGET)

install:
	cp $(TARGET) $(PREFIX)/bin


