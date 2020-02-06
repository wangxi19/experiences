ifneq ($(dbg), 1)
	CPPFLAGS += -O3
else
	CPPFLAGS += -g -O0
endif

CPPFLAGS += -fPIC
CXX = gcc

INCLUDE = -I./

CPPFLAGS += -c $(INCLUDE) 

#SRCS = bpf_filter.c gencode.c grammar.c optimize.c scanner.c
OBJS = bpf_filter.o gencode.o grammar.o optimize.o scanner.o

TARGET = libbpf.so

all:$(TARGET)

$(TARGET): $(filter %.o, $(OBJS))
	$(CXX) -shared -o $@ $^

$(filter %.o, $(OBJS)): %.o: %.c
	$(CXX) $(CPPFLAGS) $< -o $@

clean:
	$(RM) $(TARGET)
	$(RM) $(filter %.o, $(OBJS))
