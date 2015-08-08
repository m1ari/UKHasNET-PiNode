CFLAGS=-O2 -g -Wall -std=c++0x
#  -std=gnu++11 

CC=g++

LIBS=-ljansson 
	# -lncurses -lpthread -lpqxx -lpq

OBJS=spi.o pinode.o 

TARGETS=pinode

all: $(TARGETS)

pinode: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp %.h Makefile
	$(CXX) -c $(CFLAGS) -o $@ $<

$(IRC_OBJS): %.o: %.cpp %.h Makefile
	$(CXX) -c $(CFLAGS) -o $@ $<

$(OBJS): %.o: %.cpp %.h Makefile
	$(CXX) -c $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm $(OBJS) $(IRC_OBJS) $(DB_OBJS) $(TARGETS)
