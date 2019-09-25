CPPFLAGS = -static
CPPFLAGS += -std=c++11
ifneq ($(dbg), 1)
    CPPFLAGS += -O3
else
    CPPFLAGS += -g -O0
endif

CXX = g++

INCLUDE = -I./
LIBS = -lc

CPPFLAGS += $(INCLUDE) $(LIBS)

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:%.cpp=%.o)

TARGET = sshscanner

all: $(TARGET)

$(TARGET): $(filter %.o, $(OBJS))
	$(CXX) $(CPPFLAGS) $^ -o $@

$(filter %.o, $(OBJS)): %.o: %.cpp
	$(CXX) -c $< -o $@

clean:
	$(RM) $(TARGET)
	$(RM) $(filter %.o, $(OBJS))

