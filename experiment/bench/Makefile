CC = g++
CPPFLAGS =  -g -fsanitize=address -std=c++14 -Wno-unused-result -O3
# CPPFLAGS = -g -std=c++14 -Wno-unused-result -O3

CPPFILES = $(shell find . -maxdepth 2 -name "*.cpp")
# CPPFILES = $(shell find . ! -path "*list*" | grep .cpp)
OBJS = $(CPPFILES:.cpp=.o)
INCLUDES = $(CPPFILES:.cpp=.d)

all: bench

include $(INCLUDES)

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -MM -MT $*.o $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

bench: $(OBJS)
	$(CC) -o bench_start -g $^ -pthread -lhiredis -lssh -fsanitize=address
# $(CC) -o bench_start $^ -pthread -lhiredis -lssh



	
.PHONY: stop clean run

clean:
	rm -rf bench_start $(OBJS) $(INCLUDES) *.rdb *.log
	
run: bench
	@rm -rf *.rdb *.log
	@./bench_start